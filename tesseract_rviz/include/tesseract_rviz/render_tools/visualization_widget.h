/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TESSERACT_RVIZ_ROBOT_H_
#define TESSERACT_RVIZ_ROBOT_H_

#include <tesseract_common/macros.h>
TESSERACT_COMMON_IGNORE_WARNINGS_PUSH
#include <rviz_default_plugins/robot/link_updater.hpp>

#include <string>
#include <map>

#ifndef Q_MOC_RUN
#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <OgreAny.h>
#include "rviz_common/properties/property_tree_widget.hpp"
#endif

TESSERACT_COMMON_IGNORE_WARNINGS_POP

#ifndef Q_MOC_RUN
#include <tesseract_scene_graph/graph.h>
#include <tesseract_scene_graph/link.h>
#include <tesseract_scene_graph/joint.h>
#endif

namespace Ogre
{
class SceneManager;
class Entity;
class SceneNode;
class Quaternion;
class Any;
class RibbonTrail;
}  // namespace Ogre

namespace rviz_common
{
class Object;
class Axes;
class DisplayContext;
} //namespace rviz_common

namespace rviz_common::properties
{
class Property;
class EnumProperty;
class BoolProperty;
}  // namespace rviz_common::properties

namespace tf
{
class TransformListener;
}

namespace tesseract_rviz
{
class VisualizationWidget;
class LinkWidget;
class JointWidget;

template <typename Key, typename Value>
using AlignedMap = std::map<Key, Value, std::less<Key>, Eigen::aligned_allocator<std::pair<const Key, Value>>>;

using TransformMap = AlignedMap<std::string, Eigen::Isometry3d>;

/**
 * \class VisualizationWidget
 *
 * A helper class to draw a representation of a environment.  Can display either the visual models of
 * the environment, or the collision models.
 */
class VisualizationWidget : public QObject
{
  Q_OBJECT
public:
  using Ptr = std::shared_ptr<VisualizationWidget>;
  using ConstPtr = std::shared_ptr<const VisualizationWidget>;

  VisualizationWidget(Ogre::SceneNode* root_node,
                      rviz_common::DisplayContext* context,
                      std::string name,
                      rviz_common::properties::Property* parent_property);
  virtual ~VisualizationWidget();

  /**
   * \brief Loads meshes/primitives from a robot description.  Calls clear() before loading.
   *
   * @param visual Whether or not to load the visual representation
   * @param collision Whether or not to load the collision representation
   * @param only_active Whether to only show active links (Used when visualizing trajectory)
   */
  virtual void initialize(bool visual = true, bool collision = true, bool show_active = true, bool show_static = true);

  /**
   * \brief Clears all data loaded from a URDF
   */
  virtual void clear();

  /**
   * @brief Check if initialize has been called
   * @return True if initialized, otherwise false;
   */
  virtual bool isInitialized() const;

  /**
   * @brief Adds a link to the visualization
   * @param link The link to be added
   * @param replace_allowed Link replacing is allowed
   * @return Return False if a link with the same name already exists and replace is not allowed, otherwise true
   */
  virtual bool addLink(const tesseract_scene_graph::Link& link, bool replace_allowed = false);

  /**
   * @brief Removes a link from the visualization
   * @param name Name of the link to be removed
   * @return Return False if a link does not exists, otherwise true
   */
  virtual bool removeLink(const std::string& name);

  /**
   * @brief Move a link
   * @param joint The joint describing the move
   * @return Return False if a joint name already exists or if links do not exist, otherwise true
   */
  virtual bool moveLink(const tesseract_scene_graph::Joint& joint);

  /**
   * @brief Adds joint to the visualization
   * @param joint The joint to be added
   * @param replace Indicate if this should replace an existing joint
   * @return Return False if parent or child link does not exists and if joint name already exists in the graph and
   * replace is not allowed, otherwise true
   */
  virtual bool addJoint(const tesseract_scene_graph::Joint& joint, bool replace = false);

  /**
   * @brief Removes a joint from the visualization
   * @param name Name of the joint to be removed
   * @return Return False if a joint does not exists, otherwise true
   */
  virtual bool removeJoint(const std::string& name);

  /**
   * @brief Merge a graph into the current graph
   * @param scene_graph Const ref to the graph to be merged (said graph will be copied)
   * @param prefix string Will be prepended to every link and joint of the merged graph
   * @return Return False if any link or joint name collides with current environment, otherwise True
   * Merge a subgraph into the current environment, considering that the root of the merged graph is attached to the
   * root of the environment by a fixed joint and no displacement. Every joint and link of the subgraph will be copied
   * into the environment graph. The prefix argument is meant to allow adding multiple copies of the same subgraph with
   * different names
   */
  virtual bool addSceneGraph(const tesseract_scene_graph::SceneGraph& scene_graph, const std::string& prefix = "");

  /**
   * @brief Merge a graph into the current environment
   * @param scene_graph Const ref to the graph to be merged (said graph will be copied)
   * @param joint The joint that connects current environment with the inserted graph
   * @param prefix string Will be prepended to every link and joint of the merged graph
   * @return Return False if any link or joint name collides with current environment, otherwise True
   * Merge a subgraph into the current environment. Every joint and link of the subgraph will be copied into the
   * environment graph. The prefix argument is meant to allow adding multiple copies of the same subgraph with different
   * names
   */
  virtual bool addSceneGraph(const tesseract_scene_graph::SceneGraph& scene_graph,
                             const tesseract_scene_graph::Joint& joint,
                             const std::string& prefix = "");

  /**
   * @brief Move a joint from one link to another
   *
   *        All child links & joints should follow
   *
   * @param joint_name The name of the joint to move
   * @param new_parent_link The name of the link to move to.
   * @return Return False if parent_link does not exists, otherwise true
   */
  virtual bool moveJoint(const std::string& joint_name, const std::string& parent_link);

  /** @brief Changes the "origin" transform of the joint
   * @param name Name of the joint to be changed
   * @param new_origin The new transform associated with the joint
   * @return
   */
  virtual bool changeJointOrigin(const std::string& name, const Eigen::Isometry3d& new_origin);

  /**
   * @brief Disable collision between two collision objects
   * @param link_name1 Collision object name
   * @param link_name2 Collision object name
   * @param reason The reason for disabling collison
   */
  virtual void addAllowedCollision(const std::string& link_name1,
                                   const std::string& link_name2,
                                   const std::string& reason);

  /**
   * @brief Remove disabled collision pair from allowed collision matrix
   * @param link_name1 Collision object name
   * @param link_name2 Collision object name
   */
  virtual void removeAllowedCollision(const std::string& link_name1, const std::string& link_name2);

  /**
   * @brief Remove disabled collision for any pair with link_name from allowed collision matrix
   * @param link_name Collision object name
   */
  virtual void removeAllowedCollision(const std::string& link_name);

  /** @brief Update visualization widget. This must be called after finished adding or removing links */
  virtual void update();

  /**
   * @brief Update visualizations link transforms
   * @param transforms The transforms to update
   */
  virtual void update(const TransformMap& transforms);

  /**
   * \brief Set the robot as a whole to be visible or not
   * @param visible Should we be visible?
   */
  virtual void setVisible(bool visible);

  /**
   * \brief Set whether the visual meshes of the robot should be visible
   * @param visible Whether the visual meshes of the robot should be visible
   */
  void setVisualVisible(bool visible);

  /**
   * \brief Set whether the collision meshes/primitives of the robot should be visible
   * @param visible Whether the collision meshes/primitives should be visible
   */
  void setCollisionVisible(bool visible);

  /**
   * @brief Set the current state visibility
   * @param visible True/False
   */
  void setCurrentStateVisible(bool visible);

  /**
   * @brief Set the start state visibility
   * @param visible True/False
   */
  void setStartStateVisible(bool visible);

  /**
   * @brief Set the start states transforms
   * @param transforms The transforms of the start state
   */
  void setStartState(const TransformMap& transforms);

  /**
   * @brief Set the end state visibility
   * @param visible True/False
   */
  void setEndStateVisible(bool visible);

  /**
   * @brief Set the end states transforms
   * @param transforms The transforms of the start state
   */
  void setEndState(const TransformMap& transforms);

  /**
   * @brief Set the trajectory visibility
   * @param visible True/False
   */
  void setTrajectoryVisible(bool visible);

  /**
   * \brief Returns whether anything is visible
   */
  bool isVisible();
  /**
   * \brief Returns whether or not the visual representation is set to be visible
   * To be visible this and isVisible() must both be true.
   */
  bool isVisualVisible();
  /**
   * \brief Returns whether or not the collision representation is set to be visible
   * To be visible this and isVisible() must both be true.
   */
  bool isCollisionVisible();

  bool isCurrentStateVisible();

  bool isStartStateVisible();

  bool isEndStateVisible();

  bool isTrajectoryVisible();

  void setLinkCollisionEnabled(const std::string& name, bool enabled);
  void setLinkVisibleEnabled(const std::string& name, bool enabled);

  void setAlpha(float a);
  float getAlpha() { return alpha_; }
  LinkWidget* getRootLink() { return root_link_; }
  LinkWidget* getLink(const std::string& name);
  JointWidget* getJoint(const std::string& name);

  typedef std::map<std::string, LinkWidget*> M_NameToLink;
  typedef std::map<std::string, JointWidget*> M_NameToJoint;
  const M_NameToLink& getLinks() const { return links_; }
  const M_NameToJoint& getJoints() const { return joints_; }
  const std::string& getName() { return name_; }
  Ogre::SceneNode* getVisualNode() { return root_visual_node_; }
  Ogre::SceneNode* getCollisionNode() { return root_collision_node_; }
  Ogre::SceneNode* getOtherNode() { return root_other_node_; }
  Ogre::SceneManager* getSceneManager() { return scene_manager_; }
  rviz_common::DisplayContext* getDisplayContext() { return context_; }
  virtual void setPosition(const Ogre::Vector3& position);
  virtual void setOrientation(const Ogre::Quaternion& orientation);
  virtual void setScale(const Ogre::Vector3& scale);
  virtual const Ogre::Vector3& getPosition();
  virtual const Ogre::Quaternion& getOrientation();

  JointWidget* findParentJoint(JointWidget* joint);
  JointWidget* findParentJoint(LinkWidget* link);
  JointWidget* findChildJoint(LinkWidget* link);

  /** subclass LinkFactory and call setLinkFactory() to use a subclass of RobotLink and/or RobotJoint. */
  class LinkFactory
  {
  public:
    virtual ~LinkFactory() {}
    virtual LinkWidget*
    createLink(VisualizationWidget* env, const tesseract_scene_graph::Link& link, bool visual, bool collision);

    virtual JointWidget* createJoint(VisualizationWidget* robot, const tesseract_scene_graph::Joint& joint);
  };

  /** Call this before load() to subclass the RobotLink or RobotJoint class used in the link property.
   * Example:
   *    class MyLinkFactory : public LinkFactory
   *    {
   *        ...  // overload createLink() and/or createJoint()
   *    }
   *    ...
   *    robot->setLinkFactory(new MyLinkFactory());
   */
  void setLinkFactory(LinkFactory* link_factory);

  enum LinkTreeStyle
  {
    STYLE_LINK_LIST,  // list of all links sorted by link name
    STYLE_DEFAULT = STYLE_LINK_LIST,
    STYLE_JOINT_LIST,      // list of joints sorted by joint name
    STYLE_LINK_TREE,       // tree of links
    STYLE_JOINT_LINK_TREE  // tree of joints with links
  };

  /** Set the style of the link property. */
  void setLinkTreeStyle(LinkTreeStyle style);

  /** can be used to change the name, reparent, or add extra properties to the list of links */
  rviz_common::properties::Property* getLinkTreeProperty() { return link_tree_; }
  // set joint checkboxes and All Links Enabled checkbox based on current link enables.
  void calculateJointCheckboxes();

private Q_SLOTS:
  void changedLinkTreeStyle();
  void changedExpandTree();
  void changedHideSubProperties();
  void changedEnableAllLinks();
  void changedExpandLinkDetails();
  void changedExpandJointDetails();

protected:
  /** @brief Call RobotLink::updateVisibility() on each link. */
  void updateLinkVisibilities();

  /** remove all link and joint properties from their parents.
   * Needed before deletion and before rearranging link tree. */
  void unparentLinkProperties();

  // place sub properties under detail (or not)
  void useDetailProperty(bool use_detail);

  /** used by setLinkTreeStyle() to recursively build link & joint tree. */
  void addLinkToLinkTree(LinkTreeStyle style, rviz_common::properties::Property* parent, LinkWidget* link);
  void addJointToLinkTree(LinkTreeStyle style, rviz_common::properties::Property* parent, JointWidget* joint);

  // set the value of the EnableAllLinks property without affecting child links/joints.
  void setEnableAllLinksCheckbox(const QVariant& val);

  /** initialize style_name_map_ and link_tree_style_ options */
  void initLinkTreeStyle();
  static bool styleShowLink(LinkTreeStyle style);
  static bool styleShowJoint(LinkTreeStyle style);
  static bool styleIsTree(LinkTreeStyle style);

  Ogre::SceneManager* scene_manager_;
  tesseract_scene_graph::SceneGraph::Ptr scene_graph_;

  M_NameToLink links_;    ///< Map of name to link info, stores all loaded links.
  M_NameToJoint joints_;  ///< Map of name to joint info, stores all loaded joints.
  LinkWidget* root_link_;
  std::vector<std::string> active_links_;  ///< This is a list of active links
  bool load_visual_;                       ///< Indicate if visual geometries should be loaded
  bool load_collision_;                    ///< Indicate if collision geometries should be loaded
  bool load_active_;                       ///< Indicate if only active link geometries should be loaded
  bool load_static_;                       ///< Indicate if only static link geometries should be loaded

  LinkFactory* link_factory_;  ///< factory for generating links and joints

  Ogre::SceneNode* root_visual_node_;     ///< Node all our visual nodes are children of
  Ogre::SceneNode* root_collision_node_;  ///< Node all our collision nodes are children of
  Ogre::SceneNode* root_other_node_;

  bool visible_;            ///< Should we show anything at all? (affects visual, collision, axes, and trails)
  bool visual_visible_;     ///< Should we show the visual representation?
  bool collision_visible_;  ///< Should we show the collision representation?
  bool current_state_visible_;
  bool start_state_visible_;
  bool end_state_visible_;
  bool trajectory_visible_;

  rviz_common::DisplayContext* context_;
  rviz_common::properties::Property* link_tree_;
  rviz_common::properties::EnumProperty* link_tree_style_;
  rviz_common::properties::BoolProperty* expand_tree_;
  rviz_common::properties::BoolProperty* expand_link_details_;
  rviz_common::properties::BoolProperty* expand_joint_details_;
  rviz_common::properties::BoolProperty* enable_all_links_;
  //  rviz_common::properties::PropertyTreeWidget* property_widget_; TODO: Need to add this capability see view_panel.cpp in rviz as
  //  example

  std::map<LinkTreeStyle, std::string> style_name_map_;

  bool doing_set_checkbox_;  // used only inside setEnableAllLinksCheckbox()
  bool initialized_{ false };

  // true inside changedEnableAllLinks().  Prevents calculateJointCheckboxes()
  // from recalculating over and over.
  bool inChangedEnableAllLinks;

  std::string name_;
  float alpha_;
};

}  // namespace tesseract_rviz

#endif /* TESSERACT_RVIZ_ROBOT_H_ */
