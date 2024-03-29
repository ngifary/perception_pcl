#ifndef PCL_ROS__POINT_CLOUD_HPP__
#define PCL_ROS__POINT_CLOUD_HPP__

#include <rclcpp/rclcpp.hpp>
#include <pcl/point_cloud.h>
#include <pcl/type_traits.h>
#include <pcl/point_struct_traits.h>
#include <pcl/for_each_type.h>
#include <pcl/conversions.h>
#include <pcl_conversions/pcl_conversions.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

namespace pcl 
{
  namespace detail 
  {
    template<typename Stream, typename PointT>
    struct FieldStreamer
    {
      FieldStreamer(Stream& stream) : stream_(stream) {}

      template<typename U> void operator() ()
      {
        const char* name = pcl::traits::name<PointT, U>::value;
        std::uint32_t name_length = strlen(name);
        stream_.next(name_length);
        if (name_length > 0)
          memcpy(stream_.advance(name_length), name, name_length);

        std::uint32_t offset = pcl::traits::offset<PointT, U>::value;
        stream_.next(offset);

        std::uint8_t datatype = pcl::traits::datatype<PointT, U>::value;
        stream_.next(datatype);

        std::uint32_t count = pcl::traits::datatype<PointT, U>::size;
        stream_.next(count);
      }

      Stream& stream_;
    };

    template<typename PointT>
    struct FieldsLength
    {
      FieldsLength() : length(0) {}

      template<typename U> void operator() ()
      {
        std::uint32_t name_length = strlen(pcl::traits::name<PointT, U>::value);
        length += name_length + 13;
      }

      std::uint32_t length;
    };
  } // namespace pcl::detail
} // namespace pcl

namespace message_filters
{
  // In ROS 1.3.1+, we can specialize the functor used to create PointCloud<T> objects
  // on the subscriber side. This allows us to generate the mapping between message
  // data and object fields only once and reuse it.
//#if ROS_VERSION_MINIMUM(1, 3, 1)
//  template<typename T>
//  struct DefaultMessageCreator<pcl::PointCloud<T> >
//  {
//    std::shared_ptr<pcl::MsgFieldMap> mapping_;
//
//    DefaultMessageCreator()
//      : mapping_( std::make_shared<pcl::MsgFieldMap>() )
//    {
//    }
//    
//    std::shared_ptr<pcl::PointCloud<T> > operator() ()
//    {
//      std::shared_ptr<pcl::PointCloud<T> > msg (new pcl::PointCloud<T> ());
//      pcl::detail::getMapping(*msg) = mapping_;
//      return msg;
//    }
//  };
//#endif

  // https://github.com/ros2/message_filters/commit/46e1229a1d8c0ecca68e01b9cf0d8c13f9f6f87a#diff-651c2688431bf33a7ed4f0c68f9d86d2
  namespace message_traits 
  {
    /*
    template<typename T> struct MD5Sum<pcl::PointCloud<T> >
    {
      static const char* value() { return MD5Sum<sensor_msgs::PointCloud2>::value(); }
      static const char* value(const pcl::PointCloud<T>&) { return value(); }

      static const uint64_t static_value1 = MD5Sum<sensor_msgs::PointCloud2>::static_value1;
      static const uint64_t static_value2 = MD5Sum<sensor_msgs::PointCloud2>::static_value2;
      
      // If the definition of sensor_msgs/PointCloud2 changes, we'll get a compile error here.
      ROS_STATIC_ASSERT(static_value1 == 0x1158d486dd51d683ULL);
      ROS_STATIC_ASSERT(static_value2 == 0xce2f1be655c3c181ULL);
    };

    template<typename T> struct DataType<pcl::PointCloud<T> >
    {
      static const char* value() { return DataType<sensor_msgs::PointCloud2>::value(); }
      static const char* value(const pcl::PointCloud<T>&) { return value(); }
    };

    template<typename T> struct Definition<pcl::PointCloud<T> >
    {
      static const char* value() { return Definition<sensor_msgs::PointCloud2>::value(); }
      static const char* value(const pcl::PointCloud<T>&) { return value(); }
    };
    */
    // pcl point clouds message don't have a ROS compatible header
    // the specialized meta functions below (TimeStamp and FrameId)
    // can be used to get the header data.
//    template<typename T> struct HasHeader<pcl::PointCloud<T> > : std::false_type {};
//
//    template<typename T>
//    struct TimeStamp<pcl::PointCloud<T> >
//    {
//      // This specialization could be dangerous, but it's the best I can do.
//      // If this TimeStamp struct is destroyed before they are done with the
//      // pointer returned by the first functions may go out of scope, but there
//      // isn't a lot I can do about that. This is a good reason to refuse to
//      // returning pointers like this...
//      static rclcpp::Time* pointer(typename pcl::PointCloud<T> &m) {
//        header_.reset(new std_msgs::msg::Header());
//        pcl_conversions::fromPCL(m.header, *(header_));
//        stamp_.reset(new rclcpp::Time(header_->stamp));
//        return &(*stamp_);
//      }
//      static rclcpp::Time const* pointer(const typename pcl::PointCloud<T>& m) {
//        header_const_.reset(new std_msgs::msg::Header());
//        pcl_conversions::fromPCL(m.header, *(header_const_));
//        stamp_const_.reset(new rclcpp::Time(header_const_->stamp));
//        return &(*stamp_const_);
//      }
//      static rclcpp::Time value(const typename pcl::PointCloud<T>& m) {
//        return pcl_conversions::fromPCL(m.header).stamp;
//      }
//    private:
//      static std::shared_ptr<std_msgs::msg::Header> header_;
//      static std::shared_ptr<std_msgs::msg::Header> header_const_;
//      static std::shared_ptr<rclcpp::Time> stamp_;
//      static std::shared_ptr<rclcpp::Time> stamp_const_;
//    };
//
//    template<typename T>
//    struct FrameId<pcl::PointCloud<T> >
//    {
//      static std::string* pointer(pcl::PointCloud<T>& m) { return &m.header.frame_id; }
//      static std::string const* pointer(const pcl::PointCloud<T>& m) { return &m.header.frame_id; }
//      static std::string value(const pcl::PointCloud<T>& m) { return m.header.frame_id; }
//    };

  /// @todo Printer specialization in message_operations
  } // namespace message_filter::message_traits

  // https://answers.ros.org/question/303992/how-to-get-the-serialized-message-sizelength-in-ros2/
  /*
  namespace serialization 
  {
    template<typename T>
    struct Serializer<pcl::PointCloud<T> >
    {
      template<typename Stream>
      inline static void write(Stream& stream, const pcl::PointCloud<T>& m)
      {
        stream.next(m.header);
        
        // Ease the user's burden on specifying width/height for unorganized datasets
        std::uint32_t height = m.height, width = m.width;
        if (height == 0 && width == 0) {
          width = m.points.size();
          height = 1;
        }
        stream.next(height);
        stream.next(width);

        // Stream out point field metadata
        typedef typename pcl::traits::fieldList<T>::type FieldList;
        std::uint32_t fields_size = boost::mpl::size<FieldList>::value;
        stream.next(fields_size);
        pcl::for_each_type<FieldList>(pcl::detail::FieldStreamer<Stream, T>(stream));

        // Assume little-endian...
        std::uint8_t is_bigendian = false;
        stream.next(is_bigendian);

        // Write out point data as binary blob
        std::uint32_t point_step = sizeof(T);
        stream.next(point_step);
        std::uint32_t row_step = point_step * width;
        stream.next(row_step);
        std::uint32_t data_size = row_step * height;
        stream.next(data_size);
        memcpy(stream.advance(data_size), &m.points[0], data_size);

        std::uint8_t is_dense = m.is_dense;
        stream.next(is_dense);
      }

      template<typename Stream>
      inline static void read(Stream& stream, pcl::PointCloud<T>& m)
      {
        std_msgs::Header header;
        stream.next(header);
        pcl_conversions::toPCL(header, m.header);
        stream.next(m.height);
        stream.next(m.width);

        /// @todo Check that fields haven't changed!
        std::vector<sensor_msgs::PointField> fields;
        stream.next(fields);

        // Construct field mapping if deserializing for the first time
        std::shared_ptr<pcl::MsgFieldMap>& mapping_ptr = pcl::detail::getMapping(m);
        if (!mapping_ptr)
        {
          // This normally should get allocated by DefaultMessageCreator, but just in case
          mapping_ptr = std::make_shared<pcl::MsgFieldMap>();
        }
        pcl::MsgFieldMap& mapping = *mapping_ptr;
        if (mapping.empty())
          pcl::createMapping<T> (fields, mapping);

        std::uint8_t is_bigendian;
        stream.next(is_bigendian); // ignoring...
        std::uint32_t point_step, row_step;
        stream.next(point_step);
        stream.next(row_step);

        // Copy point data
        std::uint32_t data_size;
        stream.next(data_size);
        assert(data_size == m.height * m.width * point_step);
        m.points.resize(m.height * m.width);
        std::uint8_t* m_data = reinterpret_cast<std::uint8_t*>(&m.points[0]);
        // If the data layouts match, can copy a whole row in one memcpy
        if (mapping.size() == 1 &&
            mapping[0].serialized_offset == 0 &&
            mapping[0].struct_offset == 0 &&
            point_step == sizeof(T))
        {
          std::uint32_t m_row_step = sizeof(T) * m.width;
          // And if the row steps match, can copy whole point cloud in one memcpy
          if (m_row_step == row_step)
          {
            memcpy (m_data, stream.advance(data_size), data_size);
          }
          else
          {
            for (std::uint32_t i = 0; i < m.height; ++i, m_data += m_row_step)
              memcpy (m_data, stream.advance(row_step), m_row_step);
          }
        }
        else
        {
          // If not, do a lot of memcpys to copy over the fields
          for (std::uint32_t row = 0; row < m.height; ++row) {
            const std::uint8_t* stream_data = stream.advance(row_step);
            for (std::uint32_t col = 0; col < m.width; ++col, stream_data += point_step) {
              BOOST_FOREACH(const pcl::detail::FieldMapping& fm, mapping) {
                memcpy(m_data + fm.struct_offset, stream_data + fm.serialized_offset, fm.size);
              }
              m_data += sizeof(T);
            }
          }
        }

        std::uint8_t is_dense;
        stream.next(is_dense);
        m.is_dense = is_dense;
      }

      inline static std::uint32_t serializedLength(const pcl::PointCloud<T>& m)
      {
        std::uint32_t length = 0;

        length += serializationLength(m.header);
        length += 8; // height/width

        pcl::detail::FieldsLength<T> fl;
        typedef typename pcl::traits::fieldList<T>::type FieldList;
        pcl::for_each_type<FieldList>(boost::ref(fl));
        length += 4; // size of 'fields'
        length += fl.length;

        length += 1; // is_bigendian
        length += 4; // point_step
        length += 4; // row_step
        length += 4; // size of 'data'
        length += m.points.size() * sizeof(T); // data
        length += 1; // is_dense

        return length;
      }
    };
  } // namespace ros::serialization
  */

  /// @todo Printer specialization in message_operations

} // namespace message_filters

#endif
