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
 * $Id: feature.h 35422 2011-01-24 20:04:44Z rusu $
 *
 */

#ifndef PCL_ROS__FEATURES__FEATURE_HPP_
#define PCL_ROS__FEATURES__FEATURE_HPP_

// PCL includes
#include <pcl/features/feature.h>
#include <pcl_msgs/msg/point_indices.hpp>

#include "pcl_ros/pcl_node.hpp"
#include <message_filters/pass_through.h>

// PCL conversions
#include <pcl_conversions/pcl_conversions.hpp>
#include "pcl_ros/ptr_helper.hpp"

namespace pcl_ros
{
  namespace sync_policies = message_filters::sync_policies;

  ///////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////
  /** \brief @b Feature represents the base feature class. Some generic 3D operations that 
    * are applicable to all features are defined here as static methods.
    * \author Radu Bogdan Rusu
    */
  class Feature : public PCLNode
  {
    public:
      typedef pcl::KdTree<pcl::PointXYZ> KdTree;
      typedef pcl::KdTree<pcl::PointXYZ>::Ptr KdTreePtr;

      typedef pcl::PointCloud<pcl::PointXYZ> PointCloudIn;
      typedef PointCloudIn::Ptr PointCloudInPtr;
      typedef PointCloudIn::ConstPtr PointCloudInConstPtr;

      typedef std::shared_ptr <std::vector<int> > IndicesPtr;
      typedef std::shared_ptr <const std::vector<int> > IndicesConstPtr;

      /** \brief Empty constructor. */
      Feature (std::string node_name, const rclcpp::NodeOptions& options);

    protected:
      /** \brief The input point cloud dataset. */
      //PointCloudInConstPtr input_;

      /** \brief A pointer to the vector of point indices to use. */
      //IndicesConstPtr indices_;

      /** \brief An input point cloud describing the surface that is to be used for nearest neighbors estimation. */
      //PointCloudInConstPtr surface_;

      /** \brief A pointer to the spatial search object. */
      KdTreePtr tree_;
      // pcl::search::KdTree<pcl::PointXYZ>::Ptr tree 
      /** \brief The number of K nearest neighbors to use for each point. */
      int k_;

      /** \brief The nearest neighbors search radius for each point. */
      double search_radius_;

      // ROS node attributes
      /** \brief The surface PointCloud subscriber filter. */
      message_filters::Subscriber<PointCloudIn> sub_surface_filter_;
      
      /** \brief The input PointCloud subscriber. */
      rclcpp::Subscription<PointCloudIn>::SharedPtr sub_input_;


      /** \brief Set to true if the node needs to listen for incoming point clouds representing the search surface. */
      bool use_surface_;

      /** \brief Parameter for the spatial locator tree. By convention, the values represent:
        * 0: ANN (Approximate Nearest Neigbor library) kd-tree
        * 1: FLANN (Fast Library for Approximate Nearest Neighbors) kd-tree
        * 2: Organized spatial dataset index
        */
      int spatial_locator_type_;

      /** \brief Publish an empty point cloud of the feature output type. */
      virtual void emptyPublish (const PointCloudInConstPtr &cloud) = 0;

      /** \brief Compute the feature and publish it. Internal method. */
      virtual void computePublish (const PointCloudInConstPtr &cloud, 
                                   const PointCloudInConstPtr &surface,
                                   const IndicesPtr &indices) = 0;

      /** \brief Null passthrough filter, used for pushing empty elements in the 
        * synchronizer */
      message_filters::PassThrough<PointIndices> nf_pi_;
      message_filters::PassThrough<PointCloudIn> nf_pc_;

      /** \brief Input point cloud callback.
        * Because we want to use the same synchronizer object, we push back
        * empty elements with the same timestamp.
        */
      inline void
      input_callback (const PointCloudInConstPtr &input)
      {
        PointIndices indices;
        indices.header.stamp = pcl_conversions::fromPCL(input->header).stamp;
        PointCloudIn cloud;
        cloud.header.stamp = input->header.stamp;
        nf_pc_.add (cloud.makeShared());
        nf_pi_.add (std::make_shared<PointIndices> (indices));
      }

    private:
      /** \brief Synchronized input, surface, and point indices.*/
      std::shared_ptr <message_filters::Synchronizer<sync_policies::ApproximateTime<PointCloudIn, PointCloudIn, PointIndices> > > sync_input_surface_indices_a_;
      std::shared_ptr <message_filters::Synchronizer<sync_policies::ExactTime<PointCloudIn, PointCloudIn, PointIndices> > > sync_input_surface_indices_e_;


      virtual void subscribe ();
      virtual void unsubscribe ();
      /** \brief Input point cloud callback. Used when \a use_indices and \a use_surface are set.
        * \param cloud the pointer to the input point cloud
        * \param cloud_surface the pointer to the surface point cloud
        * \param indices the pointer to the input point cloud indices
        */
      void input_surface_indices_callback (const PointCloudInConstPtr &cloud, 
                                           const PointCloudInConstPtr &cloud_surface, 
                                           const PointIndicesConstPtr &indices);
    
      rclcpp::Publisher<PointCloud2>::SharedPtr pub_output_;
    
    public:
      EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  };

  //////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////
  class FeatureFromNormals : public Feature
  {
    public:
      typedef sensor_msgs::msg::PointCloud2 PointCloud2;

      typedef pcl::PointCloud<pcl::Normal> PointCloudN;
      typedef PointCloudN::Ptr PointCloudNPtr;
      typedef PointCloudN::ConstPtr PointCloudNConstPtr;

      FeatureFromNormals (std::string node_name, const rclcpp::NodeOptions& options);

    protected:
      /** \brief A pointer to the input dataset that contains the point normals of the XYZ dataset. */
      PointCloudNConstPtr normals_;

      /** \brief Publish an empty point cloud of the feature output type. */
      virtual void emptyPublish (const PointCloudInConstPtr &cloud) = 0;

      /** \brief Compute the feature and publish it. */
      virtual void computePublish (const PointCloudInConstPtr &cloud, 
                                   const PointCloudNConstPtr &normals,
                                   const PointCloudInConstPtr &surface,
                                   const IndicesPtr &indices) = 0;

    private:
      /** \brief The normals PointCloud subscriber filter. */
      message_filters::Subscriber<PointCloudN> sub_normals_filter_;

      /** \brief Synchronized input, normals, surface, and point indices.*/
      std::shared_ptr<message_filters::Synchronizer<sync_policies::ApproximateTime<PointCloudIn, PointCloudN, PointCloudIn, PointIndices> > > sync_input_normals_surface_indices_a_;
      std::shared_ptr<message_filters::Synchronizer<sync_policies::ExactTime<PointCloudIn, PointCloudN, PointCloudIn, PointIndices> > > sync_input_normals_surface_indices_e_;
    
      virtual void subscribe ();
      virtual void unsubscribe ();
    
      /** \brief Internal method. */
      void computePublish (const PointCloudInConstPtr &, 
                           const PointCloudInConstPtr &,
                           const IndicesPtr &) {} // This should never be called

      /** \brief Input point cloud callback. Used when \a use_indices and \a use_surface are set.
        * \param cloud the pointer to the input point cloud
        * \param cloud_normals the pointer to the input point cloud normals
        * \param cloud_surface the pointer to the surface point cloud
        * \param indices the pointer to the input point cloud indices
        */
      void input_normals_surface_indices_callback (const PointCloudInConstPtr &cloud, 
                                                   const PointCloudNConstPtr &cloud_normals, 
                                                   const PointCloudInConstPtr &cloud_surface, 
                                                   const PointIndicesConstPtr &indices);

    public:
      EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  };
}  // namespace pcl_ros

#endif  // PCL_ROS__FEATURES__FEATURE_HPP_
