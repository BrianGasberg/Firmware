/*************************************************************************
 * Copyright (c) 2014 Group 731 Aalborg University. All rights reserved.
 * Author:   Group 731 <14gr731@es.aau.dk>
 *************************************************************************/
/*
 * @file main.c
 * 
 * Implementation of an quadrotor formation control app for use in the 
 * semester project.
 *
 */

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <time.h>
#include <drivers/drv_hrt.h>

#include <mavlink/mavlink_log.h>

#include <uORB/uORB.h>
#include <uORB/topics/quad_att_sp.h>
#include <uORB/topics/quad_formation_msg.h>

#include <geo/geo.h>

#include <systemlib/param/param.h>
#include <systemlib/pid/pid.h>
#include <systemlib/perf_counter.h>
#include <systemlib/systemlib.h>
#include <systemlib/err.h>

/* Function prototypes */
__EXPORT int hover_test_main(int argc, char *argv[]);
int hover_test_thread_main(int argc, char *argv[]);
static void usage(const char *reason);
int getData(int *pqmsg_sub, struct quad_formation_msg_s *pqmsg);

/* Global variables */
bool thread_should_exit = false;
bool thread_running = false;
int daemon_task;

int hover_test_thread_main(int argc, char *argv[]) {

        warnx("[hover_test] started\n");

        struct quad_formation_msg_s qmsg;
        memset(&qmsg, 0, sizeof(qmsg));
        struct quad_att_sp_s sp;
        memset(&sp, 0, sizeof(sp));

        int qmsg_sub = orb_subscribe(ORB_ID(quad_formation_msg));
        orb_copy(ORB_ID(quad_formation_msg), qmsg_sub, &qmsg);

        orb_advert_t quad_att_sp_pub = orb_advertise(ORB_ID(quad_att_sp), &sp);

        struct pollfd fds[1];
        fds[0].fd = qmsg_sub;
        fds[0].events = POLLIN;

        while(!thread_should_exit) {

                int ret_qmsg = poll(fds, 1, 1000);

                if (ret_qmsg < 0) {
			warnx("poll cmd error");
		} else if (ret_qmsg == 0) {
			/* printf("[hover_test] nothing received\n"); */
		} else if (fds[0].revents & POLLIN) {
                        orb_copy(ORB_ID(quad_formation_msg), qmsg_sub, &qmsg);

                        if (qmsg.cmd_id == (enum QUAD_MSG_CMD)QUAD_MSG_CMD_START) {
                                printf("[hover_test] start\n");
                                sp.cmd = (enum QUAD_ATT_CMD)QUAD_ATT_CMD_START;
                                sp.thrust = (double)1000;
                                orb_publish(ORB_ID(quad_att_sp), quad_att_sp_pub, &sp);
                        } else if (qmsg.cmd_id == (enum QUAD_MSG_CMD)QUAD_MSG_CMD_STOP){
                                printf("[hover_test] stop\n");
                                sp.cmd = (enum QUAD_ATT_CMD)QUAD_ATT_CMD_STOP;
                                sp.thrust = 0.f;
                                orb_publish(ORB_ID(quad_att_sp), quad_att_sp_pub, &sp);
                        }
                }
        }
}

static void usage(const char *reason) {
        if (reason)
                fprintf(stderr, "%s\n", reason);

        fprintf(stderr, "usage: hover_test {start|stop|status}\n\n");
        exit(1);
}

int hover_test_main(int argc, char *argv[]) {
        if (argc < 1)
		usage("missing argument");

	if (!strcmp(argv[1], "start")) {

		if (thread_running) {
			printf("hover test already running\n");

			exit(0);
		}

		thread_should_exit = false;
		daemon_task = task_spawn_cmd("hover_test",
                                             SCHED_DEFAULT,
                                             SCHED_PRIORITY_MAX - 20,
                                             2048,
                                             hover_test_thread_main,
                                             (argv) ? (const char **)&argv[2] : (const char **)NULL);
		thread_running = true;
		exit(0);
	}

	if (!strcmp(argv[1], "stop")) {
		thread_should_exit = true;
		exit(0);
	}

	if (!strcmp(argv[1], "status")) {
		if (thread_running) {
			printf("\thover test is running\n");

		} else {
			printf("\thover_test not started\n");
		}

		exit(0);
	}

	usage("unrecognized command");
	exit(1);
}
