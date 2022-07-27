#include <tesseract_rviz/environment_plugin/environment_monitor_properties.h>
#include <tesseract_rviz/environment_plugin/ros_environment_widget.h>
#include <tesseract_rviz/environment_plugin/conversions.h>
#include <tesseract_qt/environment/environment_widget_config.h>

#include <tesseract_monitoring/environment_monitor.h>

#include <tesseract_msgs/msg/environment_state.hpp>
#include <sensor_msgs/msg/joint_state.hpp>

#include <rviz_common/display.hpp>
#include <rviz_common/properties/ros_topic_property.hpp>
#include <rviz_common/properties/string_property.hpp>
#include <rviz_common/properties/enum_property.hpp>
#include <rviz_common//panel_dock_widget.hpp>

#include <unordered_map>

namespace tesseract_rviz
{
struct EnvironmentMonitorPropertiesPrivate
{
  std::shared_ptr<rclcpp::Node> node;
  rviz_common::Display* parent;
  rviz_common::properties::Property* main_property;
  std::string monitor_namespace;
  tesseract_monitoring::ROSEnvironmentMonitor::Ptr monitor;
  tesseract_rviz::ROSEnvironmentWidget* widget;
  std::unordered_map<std::string, tesseract_gui::EnvironmentWidgetConfig::Ptr> configs;

  rviz_common::properties::EnumProperty* display_mode_property;
  rviz_common::properties::StringProperty* urdf_description_string_property;
  rviz_common::properties::RosTopicProperty* environment_topic_property;
  rviz_common::properties::RosTopicProperty* joint_state_topic_property;
};

EnvironmentMonitorProperties::EnvironmentMonitorProperties(rviz_common::Display* parent,
                                                           std::string monitor_namespace,
                                                           rviz_common::properties::Property* main_property)
  : data_(std::make_unique<EnvironmentMonitorPropertiesPrivate>())
{
  data_->parent = parent;
  data_->monitor_namespace = monitor_namespace;

  data_->main_property = main_property;
  if (data_->main_property == nullptr)
    data_->main_property = data_->parent;

  data_->display_mode_property = new rviz_common::properties::EnumProperty("Display Mode",
                                                        "URDF",
                                                        "Leverage URDF or connect to monitor namespace",
                                                        data_->main_property,
                                                        SLOT(onDisplayModeChanged()),
                                                        this);

  data_->display_mode_property->addOptionStd("URDF", 0);
  data_->display_mode_property->addOptionStd("Monitor", 1);

  data_->urdf_description_string_property = new rviz_common::properties::StringProperty("URDF Parameter",
                                                                     "robot_description",
                                                                     "The URDF parameter to use for creating the "
                                                                     "environment within RViz",
                                                                     data_->main_property,
                                                                     SLOT(onURDFDescriptionChanged()),
                                                                     this);

  data_->environment_topic_property =
      new rviz_common::properties::RosTopicProperty("Monitor Topic",
                                 "/tesseract_environment",
                                 rosidl_generator_traits::data_type<tesseract_msgs::msg::EnvironmentState>(),
                                 "This will monitor this topic for environment changes.",
                                 data_->main_property,
                                 SLOT(onEnvironmentTopicChanged()),
                                 this);

  data_->joint_state_topic_property =
      new rviz_common::properties::RosTopicProperty("Joint State Topic",
                                 "/joint_states",
                                 rosidl_generator_traits::data_type<sensor_msgs::msg::JointState>(),
                                 "This will monitor this topic for joint state changes.",
                                 data_->main_property,
                                 SLOT(onJointStateTopicChanged()),
                                 this);
}

EnvironmentMonitorProperties::~EnvironmentMonitorProperties() = default;

void EnvironmentMonitorProperties::onInitialize(ROSEnvironmentWidget* widget)
{
  data_->widget = widget;
  onDisplayModeChanged();
}

std::shared_ptr<tesseract_gui::EnvironmentWidgetConfig> EnvironmentMonitorProperties::getConfig() const
{
  if (data_->display_mode_property->getOptionInt() == 0)
  {
    auto it = data_->configs.find(data_->urdf_description_string_property->getStdString());
    if (it != data_->configs.end())
      return it->second;
  }
  else if (data_->display_mode_property->getOptionInt() == 1)
  {
    auto it = data_->configs.find(data_->environment_topic_property->getStdString());
    if (it != data_->configs.end())
      return it->second;
  }

  return nullptr;
}

void EnvironmentMonitorProperties::load(const rviz_common::Config& config)
{
  QString mode;
  if (config.mapGetString("tesseract::EnvMonitorMode", &mode))
    data_->display_mode_property->setString(mode);

  QString urdf_description;
  if (config.mapGetString("tesseract::EnvMonitorURDFDescription", &urdf_description))
    data_->urdf_description_string_property->setString(urdf_description);

  QString topic;
  if (config.mapGetString("tesseract::EnvMonitorTopic", &topic))
    data_->environment_topic_property->setString(topic);

  if (config.mapGetString("tesseract::EnvMonitorJointStateTopic", &topic))
    data_->joint_state_topic_property->setString(topic);
}

void EnvironmentMonitorProperties::save(rviz_common::Config config) const
{
  config.mapSetValue("tesseract::EnvMonitorMode", data_->display_mode_property->getString());
  config.mapSetValue("tesseract::EnvMonitorURDFDescription", data_->urdf_description_string_property->getString());
  config.mapSetValue("tesseract::EnvMonitorTopic", data_->environment_topic_property->getString());
  config.mapSetValue("tesseract::EnvMonitorJointStateTopic", data_->joint_state_topic_property->getString());
}

void EnvironmentMonitorProperties::onDisplayModeChanged()
{
  if (data_->display_mode_property->getOptionInt() == 0)
  {
    data_->environment_topic_property->setHidden(true);
    data_->urdf_description_string_property->setHidden(false);
    onURDFDescriptionChanged();
  }
  else if (data_->display_mode_property->getOptionInt() == 1)
  {
    data_->urdf_description_string_property->setHidden(true);
    data_->environment_topic_property->setHidden(false);
    onEnvironmentTopicChanged();
  }
  onJointStateTopicChanged();
}

void EnvironmentMonitorProperties::onURDFDescriptionChanged()
{
  if (data_->widget == nullptr)
    return;

  auto it = data_->configs.find(data_->urdf_description_string_property->getStdString());
  if (it != data_->configs.end())
  {
    if (data_->monitor != nullptr)
      data_->monitor->shutdown();

    data_->monitor = std::make_unique<tesseract_monitoring::ROSEnvironmentMonitor>(it->second->getEnvironment(),
                                                                                   data_->monitor_namespace);
    if (data_->monitor != nullptr)
    {
      data_->widget->setConfiguration(it->second);
      onJointStateTopicChanged();
    }
  }
  else
  {
    std::string urdf_xml_string, srdf_xml_string;
    // TODO: Get urdf and srdf properties
//    data_->nh.getParam(data_->urdf_description_string_property->getStdString(), urdf_xml_string);
//    data_->nh.getParam(data_->urdf_description_string_property->getStdString() + "_semantic", srdf_xml_string);

    auto env = std::make_shared<tesseract_environment::Environment>();
    auto locator = std::make_shared<tesseract_rosutils::ROSResourceLocator>();
    if (env->init(urdf_xml_string, srdf_xml_string, locator))
    {
      if (data_->monitor != nullptr)
        data_->monitor->shutdown();

      data_->monitor = std::make_unique<tesseract_monitoring::ROSEnvironmentMonitor>(env, data_->monitor_namespace);
      if (data_->monitor != nullptr)
      {
        auto config = std::make_shared<tesseract_gui::EnvironmentWidgetConfig>();
        config->setEnvironment(env);
        data_->widget->setConfiguration(config);
        onJointStateTopicChanged();
        data_->configs[data_->urdf_description_string_property->getStdString()] = config;
      }
    }
    else
    {
      data_->parent->setStatus(rviz_common::properties::StatusProperty::Error, "Tesseract", "URDF file failed to parse");
    }
  }
}

void EnvironmentMonitorProperties::onEnvironmentTopicChanged()
{
  if (data_->widget == nullptr)
    return;

  if (data_->monitor != nullptr)
    data_->monitor->shutdown();

  auto it = data_->configs.find(data_->environment_topic_property->getStdString());
  if (it != data_->configs.end())
  {
    data_->monitor = std::make_unique<tesseract_monitoring::ROSEnvironmentMonitor>(it->second->getEnvironment(),
                                                                                   data_->monitor_namespace);
    if (data_->monitor != nullptr)
    {
      data_->widget->setConfiguration(it->second);
      onJointStateTopicChanged();
    }
  }
  else
  {
    auto env = std::make_shared<tesseract_environment::Environment>();
    data_->monitor = std::make_unique<tesseract_monitoring::ROSEnvironmentMonitor>(env, data_->monitor_namespace);

    if (data_->monitor != nullptr)
    {
      auto config = std::make_shared<tesseract_gui::EnvironmentWidgetConfig>();
      config->setEnvironment(env);
      data_->widget->setConfiguration(config);

      std::string ns = getEnvNamespaceFromTopic(data_->environment_topic_property->getStdString());
      if (!ns.empty())
        data_->monitor->startMonitoringEnvironment(ns);
      else
        data_->parent->setStatus(rviz_common::properties::StatusProperty::Error, "Tesseract", "Invalid environment monitor topic!");

      onJointStateTopicChanged();
      data_->configs[data_->environment_topic_property->getStdString()] = config;
    }
  }
}

void EnvironmentMonitorProperties::onJointStateTopicChanged()
{
  if (data_->monitor != nullptr)
    data_->monitor->startStateMonitor(data_->joint_state_topic_property->getStdString());
}

}  // namespace tesseract_rviz
