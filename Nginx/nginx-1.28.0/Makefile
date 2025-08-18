
default:	build

clean:
	rm -rf Makefile objs

.PHONY:	default clean

build:
	$(MAKE) -f objs/Makefile

install:
	$(MAKE) -f objs/Makefile install

modules:
	$(MAKE) -f objs/Makefile modules

upgrade:
	/home/shared/OJ_sys/Nginx/sbin/nginx -t

	kill -USR2 `cat /home/shared/OJ_sys/Nginx/logs/nginx.pid`
	sleep 1
	test -f /home/shared/OJ_sys/Nginx/logs/nginx.pid.oldbin

	kill -QUIT `cat /home/shared/OJ_sys/Nginx/logs/nginx.pid.oldbin`

.PHONY:	build install modules upgrade
