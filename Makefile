# @file Makefile
# @brief proctemp makefile
# @author Jeff Perry <jeffsp@gmail.com>
# @version 1.0
# @date 2013-05-01

waf:
	waf

run: waf
	./build/debug/proctempview
	./build/debug/proctemp
	./build/debug/proctemp -f
	./build/debug/proctemp -g
	./build/debug/proctemp -f -g
