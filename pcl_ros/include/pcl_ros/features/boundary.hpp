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
 * $Id: boundary.h 35361 2011-01-20 04:34:49Z rusu $
 *
 */

#ifndef PCL_ROS__FEATURES__BOUNDARY_HPP_
#define PCL_ROS__FEATURES__BOUNDARY_HPP_

#define EIGEN_YES_I_KNOW_SPARSE_MODULE_IS_NOT_STABLE_YET true

#include <pcl/features/boundary.h>
#include "pcl_ros/features/feature.hpp"

namespace pcl_ros
{
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief @b BoundaryEstimation estimates whether a set of points is lying on surface boundaries using an angle
    * criterion. The code makes use of the estimated surface normals at each point in the input dataset.
    *
    * @note The code is stateful as we do not expect this class to be multicore parallelized. Please look at
    * \a NormalEstimationOpenMP and \a NormalEstimationTBB for examples on how to extend this to parallel implementations.
    * \author Radu Bogdan Rusu
    */
  class BoundaryEstimation: public FeatureFromNormals
  {
    public:
      BoundaryEstimation(const rclcpp::NodeOptions& options);

    private:
      pcl::BoundaryEstimation<pcl::PointXYZ, pcl::Normal, pcl::Boundary> impl_;

      typedef pcl::PointCloud<pcl::Boundary> PointCloudOut;

      /** \brief Publish an empty point cloud of the feature output type. */
      void emptyPublish (const PointCloudInConstPtr &cloud);

      /** \brief Compute the feature and publish it. */
      void computePublish (const PointCloudInConstPtr &cloud,
                           const PointCloudNConstPtr &normals,
                           const PointCloudInConstPtr &surface,
                           const IndicesPtr &indices);
    
      rclcpp::Publisher<PointCloudOut>::SharedPtr pub_output_;
    
      pcl::search::KdTree<pcl::PointXYZ>::Ptr tree_;

    public:
      EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  };
}  // namespace pcl_ros

#endif  // PCL_ROS__FEATURES__BOUNDARY_HPP_
