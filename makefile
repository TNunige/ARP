all: master blackboard window drone keyboard watchdog

clean:
	rm -f Build/master Build/blackboard Build/window Build/drone Build/keyboard Build/watchdog

clean-logs:
	rm Log/*

master: master.c
	gcc master.c -o Build/master

blackboard: blackboard.c
	gcc blackboard.c -o Build/blackboard

window: window.c
	gcc window.c -o Build/window -lncurses

drone: drone.c
	gcc drone.c -o Build/drone -lm

keyboard: keyboard.c
	gcc keyboard.c -o Build/keyboard -lncurses

watchdog: watchdog.c
	gcc watchdog.c -o Build/watchdog