#!/bin/sh
echo 8 > /sys/kernel/debug/omap_mux/gpmc_a2
echo 8 > /sys/kernel/debug/omap_mux/ecap0_in_pwm0_out

./motor_controller
