/* NeoStats - IRC Statistical Services 
** Copyright (c) 1999-2004 Adam Rutter, Justin Hammond
** http://www.neostats.net/
**
**  Portions Copyright (c) 2000-2001 ^Enigma^
**
**  Portions Copyright (c) 1999 Johnathan George net@lite.net
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
**  USA
**
** NeoStats CVS Identification
** $Id: client.c 15 2004-05-18 12:30:04Z Fish $
*/

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
                     
#include "defines.h"
#include "conf.h"
#include "log.h"
#include "libcli.h"
#include "admincli.h"
#include "mythread.h"



#define MODE_CONFIG_INT		10



int cmd_test(struct cli_def *cli, char *command, char *argv[], int argc)
{
    int i;
    cli_print(cli, "called %s with \"%s\"", __FUNCTION__, command);
    cli_print(cli, "%d arguments:", argc);
    for (i = 0; i < argc; i++)
    {
	cli_print(cli, "	%s", argv[i]);
    }
    return CLI_OK;
}


int cmd_threads(struct cli_def *cli, char *command, char *argv[], int argc)
{
    int i = 1;
    lnode_t *node;
    mythreads *tid;

    MYLOCK(&mythreadengine);
    cli_print(cli, "Active Threads (%d):", (int) list_count(threads));
    node = list_first(threads);
    while (node) {
    	tid = lnode_get(node);
    	cli_print(cli, "%d) Thread: %s TID: %ld",i , tid->name, tid->tid);
    	i++;
    	node = list_next(threads, node);
    }
    MYUNLOCK(&mythreadengine);
    return CLI_OK;
}

int cmd_set(struct cli_def *cli, char *command, char *argv[], int argc)
{
    if (argc < 2)
    {
	cli_print(cli, "Specify a variable to set");
	return CLI_OK;
    }
    cli_print(cli, "Setting \"%s\" to \"%s\"", argv[0], argv[1]);
    return CLI_OK;
}

int cmd_config_int(struct cli_def *cli, char *command, char *argv[], int argc)
{
	if (argc < 1)
	{
		cli_print(cli, "Specify an interface to configure");
		return CLI_OK;
	}
	if (strcmp(argv[0], "?") == 0)
	{
		cli_print(cli, "  test0/0");
	}
	else if (strcasecmp(argv[0], "test0/0") == 0)
	{
		cli_set_configmode(cli, MODE_CONFIG_INT, "test");
	}
	else
		cli_print(cli, "Unknown interface %s", argv[0]);
	return CLI_OK;
}

int cmd_config_int_exit(struct cli_def *cli, char *command, char *argv[], int argc)
{
	cli_set_configmode(cli, MODE_CONFIG, NULL);
	return CLI_OK;
}

int check_auth(char *username, char *password)
{
    if (!strcasecmp(username, "fred") && !strcasecmp(password, "nerk"))
	return 1;
    return 0;
}

int check_enable(char *password)
{
    if (!strcasecmp(password, "topsecret"))
        return 1;
    return 0;
}

void pc(struct cli_def *cli, char *string)
{
	nlog(LOG_DEBUG1, LOG_CORE, "%s", string);
}




void *init_admin_cli(void *arg) {
    struct cli_command *c;
    struct cli_def *cli;
    int sock = (int) arg;


    thread_created("AdminCLI");
    cli = cli_init();
    cli_set_banner(cli, "MQServer Admin Interface");
    cli_set_hostname(cli, "MQ");
    cli_register_command(cli, NULL, "set", cmd_set,  PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL);
    c = cli_register_command(cli, NULL, "show", NULL,  PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    cli_register_command(cli, c, "threads", cmd_threads, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Show the threads that are currently active");


    cli_register_command(cli, NULL, "interface", cmd_config_int, PRIVILEGE_PRIVILEGED, MODE_CONFIG, "Configure an interface");
    cli_register_command(cli, NULL, "exit", cmd_config_int_exit, PRIVILEGE_PRIVILEGED, MODE_CONFIG_INT, "Exit from interface configuration");
    cli_register_command(cli, NULL, "address", cmd_test, PRIVILEGE_PRIVILEGED, MODE_CONFIG_INT, "Set IP address");

    cli_set_auth_callback(cli, check_auth);
    cli_set_enable_callback(cli, check_enable);

    cli_loop(cli, sock);
    close(sock);
    cli_done(cli);
    /* finish thread */
    destroy_thread();
    return 0;


}
