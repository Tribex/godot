/*************************************************************************/
/*  visibility_notifier_2d.cpp                                           */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2018 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2018 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "visibility_notifier_2d.h"

#include "engine.h"
#include "particles_2d.h"
#include "scene/2d/animated_sprite.h"
#include "scene/animation/animation_player.h"
#include "scene/main/viewport.h"
#include "scene/scene_string_names.h"

void VisibilityNotifier2D::_enter_viewport(Viewport *p_viewport) {

	ERR_FAIL_COND(viewports.has(p_viewport));
	viewports.insert(p_viewport);

	if (is_inside_tree() && Engine::get_singleton()->is_editor_hint())
		return;

	if (viewports.size() == 1) {
		emit_signal(SceneStringNames::get_singleton()->screen_entered);

		_screen_enter();
	}
	emit_signal(SceneStringNames::get_singleton()->viewport_entered, p_viewport);
}

void VisibilityNotifier2D::_exit_viewport(Viewport *p_viewport) {

	ERR_FAIL_COND(!viewports.has(p_viewport));
	viewports.erase(p_viewport);

	if (is_inside_tree() && Engine::get_singleton()->is_editor_hint())
		return;

	emit_signal(SceneStringNames::get_singleton()->viewport_exited, p_viewport);
	if (viewports.size() == 0) {
		emit_signal(SceneStringNames::get_singleton()->screen_exited);

		_screen_exit();
	}
}

void VisibilityNotifier2D::set_rect(const Rect2 &p_rect) {

	rect = p_rect;
	if (is_inside_tree()) {
		get_world_2d()->_update_notifier(this, get_global_transform().xform(rect));
		if (Engine::get_singleton()->is_editor_hint()) {
			update();
			item_rect_changed();
		}
	}

	_change_notify("rect");
}

Rect2 VisibilityNotifier2D::_edit_get_rect() const {

	return rect;
}

Rect2 VisibilityNotifier2D::get_rect() const {

	return rect;
}

void VisibilityNotifier2D::_notification(int p_what) {

	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {

			//get_world_2d()->
			get_world_2d()->_register_notifier(this, get_global_transform().xform(rect));
		} break;
		case NOTIFICATION_TRANSFORM_CHANGED: {

			//get_world_2d()->
			get_world_2d()->_update_notifier(this, get_global_transform().xform(rect));
		} break;
		case NOTIFICATION_DRAW: {

			if (Engine::get_singleton()->is_editor_hint()) {

				draw_rect(rect, Color(1, 0.5, 1, 0.2));
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {

			get_world_2d()->_remove_notifier(this);
		} break;
	}
}

bool VisibilityNotifier2D::is_on_screen() const {

	return viewports.size() > 0;
}

void VisibilityNotifier2D::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_rect", "rect"), &VisibilityNotifier2D::set_rect);
	ClassDB::bind_method(D_METHOD("get_rect"), &VisibilityNotifier2D::get_rect);
	ClassDB::bind_method(D_METHOD("is_on_screen"), &VisibilityNotifier2D::is_on_screen);

	ADD_PROPERTY(PropertyInfo(Variant::RECT2, "rect"), "set_rect", "get_rect");

	ADD_SIGNAL(MethodInfo("viewport_entered", PropertyInfo(Variant::OBJECT, "viewport", PROPERTY_HINT_RESOURCE_TYPE, "Viewport")));
	ADD_SIGNAL(MethodInfo("viewport_exited", PropertyInfo(Variant::OBJECT, "viewport", PROPERTY_HINT_RESOURCE_TYPE, "Viewport")));
	ADD_SIGNAL(MethodInfo("screen_entered"));
	ADD_SIGNAL(MethodInfo("screen_exited"));
}

VisibilityNotifier2D::VisibilityNotifier2D() {

	rect = Rect2(-10, -10, 20, 20);
	set_notify_transform(true);
}

//////////////////////////////////////

void VisibilityEnabler2D::_screen_enter() {

	for (Map<Node *, Variant>::Element *E = nodes.front(); E; E = E->next()) {

		_change_node_state(E->key(), true);
	}

	if (enabler[ENABLER_PARENT_PHYSICS_PROCESS] && get_parent())
		get_parent()->set_physics_process(true);
	if (enabler[ENABLER_PARENT_PROCESS] && get_parent())
		get_parent()->set_process(true);

	visible = true;
}

void VisibilityEnabler2D::_screen_exit() {

	for (Map<Node *, Variant>::Element *E = nodes.front(); E; E = E->next()) {

		_change_node_state(E->key(), false);
	}

	if (enabler[ENABLER_PARENT_PHYSICS_PROCESS] && get_parent())
		get_parent()->set_physics_process(false);
	if (enabler[ENABLER_PARENT_PROCESS] && get_parent())
		get_parent()->set_process(false);

	visible = false;
}

void VisibilityEnabler2D::_find_nodes(Node *p_node) {

	bool add = false;
	Variant meta;

	if (enabler[ENABLER_PAUSE_ANIMATIONS]) {

		AnimationPlayer *ap = Object::cast_to<AnimationPlayer>(p_node);
		if (ap) {
			add = true;
		}
	}

	if (enabler[ENABLER_PAUSE_ANIMATED_SPRITES]) {

		AnimatedSprite *as = Object::cast_to<AnimatedSprite>(p_node);
		if (as) {
			add = true;
		}
	}

	if (enabler[ENABLER_PAUSE_PARTICLES]) {

		Particles2D *ps = Object::cast_to<Particles2D>(p_node);
		if (ps) {
			add = true;
		}
	}

	if (add) {

		p_node->connect(SceneStringNames::get_singleton()->tree_exiting, this, "_node_removed", varray(p_node), CONNECT_ONESHOT);
		nodes[p_node] = meta;
		_change_node_state(p_node, false);
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *c = p_node->get_child(i);
		if (c->get_filename() != String())
			continue; //skip, instance

		_find_nodes(c);
	}
}

void VisibilityEnabler2D::_notification(int p_what) {

	if (p_what == NOTIFICATION_ENTER_TREE) {

		if (Engine::get_singleton()->is_editor_hint())
			return;

		Node *from = this;
		//find where current scene starts
		while (from->get_parent() && from->get_filename() == String())
			from = from->get_parent();

		_find_nodes(from);

		if (enabler[ENABLER_PARENT_PHYSICS_PROCESS] && get_parent())
			get_parent()->set_physics_process(false);
		if (enabler[ENABLER_PARENT_PROCESS] && get_parent())
			get_parent()->set_process(false);
	}

	if (p_what == NOTIFICATION_EXIT_TREE) {

		if (Engine::get_singleton()->is_editor_hint())
			return;

		for (Map<Node *, Variant>::Element *E = nodes.front(); E; E = E->next()) {

			if (!visible)
				_change_node_state(E->key(), true);
			E->key()->disconnect(SceneStringNames::get_singleton()->tree_exiting, this, "_node_removed");
		}

		nodes.clear();
	}
}

void VisibilityEnabler2D::_change_node_state(Node *p_node, bool p_enabled) {

	ERR_FAIL_COND(!nodes.has(p_node));

	{
		AnimationPlayer *ap = Object::cast_to<AnimationPlayer>(p_node);

		if (ap) {

			ap->set_active(p_enabled);
		}
	}
	{
		AnimatedSprite *as = Object::cast_to<AnimatedSprite>(p_node);

		if (as) {

			if (p_enabled)
				as->play();
			else
				as->stop();
		}
	}

	{
		Particles2D *ps = Object::cast_to<Particles2D>(p_node);

		if (ps) {

			ps->set_emitting(p_enabled);
		}
	}
}

void VisibilityEnabler2D::_node_removed(Node *p_node) {

	if (!visible)
		_change_node_state(p_node, true);
	//changed to one shot, not needed
	//p_node->disconnect(SceneStringNames::get_singleton()->exit_scene,this,"_node_removed");
	nodes.erase(p_node);
}

String VisibilityEnabler2D::get_configuration_warning() const {
#ifdef TOOLS_ENABLED
	if (is_inside_tree() && get_parent() && (get_parent()->get_filename() == String() && get_parent() != get_tree()->get_edited_scene_root())) {
		return TTR("VisibilityEnable2D works best when used with the edited scene root directly as parent.");
	}
#endif
	return String();
}

void VisibilityEnabler2D::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_enabler", "enabler", "enabled"), &VisibilityEnabler2D::set_enabler);
	ClassDB::bind_method(D_METHOD("is_enabler_enabled", "enabler"), &VisibilityEnabler2D::is_enabler_enabled);
	ClassDB::bind_method(D_METHOD("_node_removed"), &VisibilityEnabler2D::_node_removed);

	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "pause_animations"), "set_enabler", "is_enabler_enabled", ENABLER_PAUSE_ANIMATIONS);
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "pause_particles"), "set_enabler", "is_enabler_enabled", ENABLER_PAUSE_PARTICLES);
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "pause_animated_sprites"), "set_enabler", "is_enabler_enabled", ENABLER_PAUSE_ANIMATED_SPRITES);
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "process_parent"), "set_enabler", "is_enabler_enabled", ENABLER_PARENT_PROCESS);
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "physics_process_parent"), "set_enabler", "is_enabler_enabled", ENABLER_PARENT_PHYSICS_PROCESS);

	BIND_ENUM_CONSTANT(ENABLER_PAUSE_ANIMATIONS);
	BIND_ENUM_CONSTANT(ENABLER_PAUSE_PARTICLES);
	BIND_ENUM_CONSTANT(ENABLER_PARENT_PROCESS);
	BIND_ENUM_CONSTANT(ENABLER_PARENT_PHYSICS_PROCESS);
	BIND_ENUM_CONSTANT(ENABLER_PAUSE_ANIMATED_SPRITES);
	BIND_ENUM_CONSTANT(ENABLER_MAX);
}

void VisibilityEnabler2D::set_enabler(Enabler p_enabler, bool p_enable) {

	ERR_FAIL_INDEX(p_enabler, ENABLER_MAX);
	enabler[p_enabler] = p_enable;
}
bool VisibilityEnabler2D::is_enabler_enabled(Enabler p_enabler) const {

	ERR_FAIL_INDEX_V(p_enabler, ENABLER_MAX, false);
	return enabler[p_enabler];
}

VisibilityEnabler2D::VisibilityEnabler2D() {

	for (int i = 0; i < ENABLER_MAX; i++)
		enabler[i] = true;
	enabler[ENABLER_PARENT_PROCESS] = false;
	enabler[ENABLER_PARENT_PHYSICS_PROCESS] = false;

	visible = false;
}
