#!/usr/bin/env python

import champlain
import clutter
import cluttercairo
import cairo
import math
import gobject

MARKER_SIZE = 10

def create_marker():
  
	orange = clutter.Color(0xf3, 0x94, 0x07, 0xbb)
  	white = clutter.Color(0xff, 0xff, 0xff, 0xff)
	timeline = clutter.Timeline()
	
	marker = champlain.Marker()
	bg = cluttercairo.CairoTexture(MARKER_SIZE, MARKER_SIZE)
	cr = bg.cairo_create()

	cr.set_source_rgb(0, 0, 0)
	cr.arc(MARKER_SIZE / 2.0, MARKER_SIZE / 2.0, MARKER_SIZE / 2.0, 0, 2 * math.pi)
	cr.close_path()

	cr.set_source_rgba(0.1, 0.1, 0.9, 1.0)
	cr.fill()

	del cr

	marker.add(bg)
	bg.set_anchor_point_from_gravity(clutter.GRAVITY_CENTER)
  	bg.set_position(0, 0)

	bg = cluttercairo.CairoTexture(2 * MARKER_SIZE, 2 * MARKER_SIZE)
	cr = bg.cairo_create()

	cr.set_source_rgb(0, 0, 0)
	cr.arc(MARKER_SIZE, MARKER_SIZE, 0.9 * MARKER_SIZE, 0, 2 * math.pi)
	cr.close_path()

	cr.set_line_width(2.0)
	cr.set_source_rgba(0.1, 0.1, 0.7, 1.0)
	cr.stroke()

	del cr

	marker.add(bg)
	bg.lower_bottom()
	bg.set_position(0, 0)
	bg.set_anchor_point_from_gravity(clutter.GRAVITY_CENTER)

	timeline.set_duration(1000)
	timeline.set_loop(True)

	alpha = clutter.Alpha()
	alpha.set_timeline(timeline)
	alpha.set_func(clutter.sine_inc_func)

	behaviour = clutter.BehaviourScale(0.5, 0.5, 2.0, 2.0)
  	
	behaviour.set_alpha(alpha)
	behaviour.apply(bg)

	behaviour = clutter.BehaviourOpacity(255, 0)
	behaviour.set_alpha(alpha)
	behaviour.apply(bg)

	timeline.start()

	#Set marker position on the map
	marker.set_position(45.528178, -73.563788)

	return marker;


def main():

	gobject.threads_init()

	clutter.init()
	stage = clutter.stage_get_default()
	stage.set_size(800, 600)
	
	actor = champlain.View()
	actor.set_size(800, 600)
	stage.add(actor)

	layer = champlain.Layer()
	layer.show()
	actor.add_layer(layer)

	marker = create_marker()
	layer.add(marker)

	# Finish initialising the map view
	actor.set_property("zoom-level", 5)
	actor.set_property("scroll-mode", champlain.SCROLL_MODE_KINETIC)
  
  	actor.center_on(45.466, -73.75)
	stage.show()
	clutter.main()


if __name__ == "__main__":
	main()