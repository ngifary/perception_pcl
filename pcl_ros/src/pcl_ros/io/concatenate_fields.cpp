/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: concatenate_fields.cpp 35052 2011-01-03 21:04:57Z rusu $
 *
 */

/** \author Radu Bogdan Rusu */

#include <pcl/common/io.h>
#include "pcl_ros/io/concatenate_fields.hpp"

#include <pcl_conversions/pcl_conversions.hpp>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
pcl_ros::PointCloudConcatenateFieldsSynchronizer::PointCloudConcatenateFieldsSynchronizer (const rclcpp::NodeOptions& options) : rclcpp::Node ("PointCloudConcatenateFieldsSynchronizerNode", options), maximum_queue_size_ (3), maximum_seconds_ (0)
{
  // ---[ Mandatory parameters
  if (!this->get_parameter ("input_messages", input_messages_))
  {
    RCLCPP_ERROR (this->get_logger(), "[%s onConstructor] Need a 'input_messages' parameter to be set before continuing!", this->get_name());
    return;
  }
  if (input_messages_ < 2)
  {
    RCLCPP_ERROR (this->get_logger(), "[%s onConstructor] Invalid 'input_messages' parameter given!", this->get_name());
    return;
  }
  // ---[ Optional parameters
  this->get_parameter ("max_queue_size", maximum_queue_size_);
  this->get_parameter ("maximum_seconds", maximum_seconds_);
  pub_output_ = this->create_publisher<sensor_msgs::msg::PointCloud2> ("output", maximum_queue_size_);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
pcl_ros::PointCloudConcatenateFieldsSynchronizer::subscribe ()
{
  // workaround ros2/rclcpp#766
  std::function<void(PointCloudPtr)> callback = std::bind(&PointCloudConcatenateFieldsSynchronizer::input_callback, this, std::placeholders::_1);
  // TODO(sloretz) what's the right QoS?
  rclcpp::QoS qos(1);
  sub_input_ = this->create_subscription<PointCloud> ("input", qos, callback);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
pcl_ros::PointCloudConcatenateFieldsSynchronizer::unsubscribe ()
{
  std::cout << "shutdown" << std::endl;
  //this->shutdown ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
pcl_ros::PointCloudConcatenateFieldsSynchronizer::input_callback (const PointCloudPtr cloud)
{
  RCLCPP_DEBUG (this->get_logger(), "[input_callback] PointCloud with %d data points (%s), stamp %d, and frame %s on topic %s received.",
                 cloud->width * cloud->height, pcl::getFieldsList (*cloud).c_str (), cloud->header.stamp.sec, cloud->header.frame_id.c_str (), "input");

  // Erase old data in the queue
  if (maximum_seconds_ > 0 && queue_.size () > 0)
  {
    while (fabs ( ( (*queue_.begin ()).first - cloud->header.stamp).seconds () ) > maximum_seconds_ && queue_.size () > 0)
    {
      /*RCLCPP_WARN (this->get_logger(), "[input_callback] Maximum seconds limit (%f) reached. Difference is %f, erasing message in queue with stamp %d.", maximum_seconds_,
                 (*queue_.begin ()).first.seconds (), fabs ( ( (*queue_.begin ()).first - cloud->header.stamp).seconds () ));*/
      queue_.erase (queue_.begin ());
    }
  }

  // Push back new data
  queue_[cloud->header.stamp].push_back (cloud);
  if ((int)queue_[cloud->header.stamp].size () >= input_messages_)
  {
    // Concatenate together and publish
    std::vector<PointCloudConstPtr> &clouds = queue_[cloud->header.stamp];
    PointCloud cloud_out = *clouds[0];

    // Resize the output dataset
    int data_size = cloud_out.data.size ();
    int nr_fields = cloud_out.fields.size ();
    int nr_points = cloud_out.width * cloud_out.height;
    for (size_t i = 1; i < clouds.size (); ++i)
    {
      assert (clouds[i]->data.size () / (clouds[i]->width * clouds[i]->height) == clouds[i]->point_step);

      if (clouds[i]->width != cloud_out.width || clouds[i]->height != cloud_out.height)
      {
        RCLCPP_ERROR (this->get_logger(), "[input_callback] Width/height of pointcloud %zu (%dx%d) differs from the others (%dx%d)!",
            i, clouds[i]->width, clouds[i]->height, cloud_out.width, cloud_out.height);
        break;
      }
      // Point step must increase with the length of each new field
      cloud_out.point_step += clouds[i]->point_step;
      // Resize data to hold all clouds
      data_size += clouds[i]->data.size ();

      // Concatenate fields
      cloud_out.fields.resize (nr_fields + clouds[i]->fields.size ());
      int delta_offset = cloud_out.fields[nr_fields - 1].offset + pcl::getFieldSize (cloud_out.fields[nr_fields - 1].datatype);
      for (size_t d = 0; d < clouds[i]->fields.size (); ++d)
      {
        cloud_out.fields[nr_fields + d] = clouds[i]->fields[d];
        cloud_out.fields[nr_fields + d].offset += delta_offset;
      }
      nr_fields += clouds[i]->fields.size ();
    }
    // Recalculate row_step
    cloud_out.row_step = cloud_out.point_step * cloud_out.width;
    cloud_out.data.resize (data_size);

    // Iterate over each point and perform the appropriate memcpys
    int point_offset = 0;
    for (int cp = 0; cp < nr_points; ++cp)
    {
      for (size_t i = 0; i < clouds.size (); ++i)
      {
        // Copy each individual point
        memcpy (&cloud_out.data[point_offset], &clouds[i]->data[cp * clouds[i]->point_step], clouds[i]->point_step);
        point_offset += clouds[i]->point_step;
      }
    }
    pub_output_->publish (*std::make_shared<const PointCloud> (cloud_out));
    queue_.erase (cloud->header.stamp);
  }

  // Clean the queue to avoid overflowing
  if (maximum_queue_size_ > 0)
  {
    while ((int)queue_.size () > maximum_queue_size_)
      queue_.erase (queue_.begin ());
  }

}

typedef pcl_ros::PointCloudConcatenateFieldsSynchronizer PointCloudConcatenateFieldsSynchronizer;
#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(PointCloudConcatenateFieldsSynchronizer)
