/* NeoStats - IRC Statistical Services
** Copyright (c) 1999-2004 Adam Rutter, Justin Hammond
** http://www.neostats.net/
**
**
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
** $Id: dns.c 1726 2004-04-18 10:34:56Z Fish $
*/


/* this file does the dns checking for adns. it provides a callback mechinism for dns lookups
** so that DNS lookups will not block. It uses the adns libary (installed in the adns directory
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "defines.h"
#include "log.h"
#include "adns.h"
#include "dns.h"

adns_state ads;

/** @brief DNS lookup Struct
 * structure containing all pending DNS lookups and the callback functions 
 */
struct dnslookup_struct {
	adns_query q;	/**< the ADNS query */
	adns_answer *a;	/**< the ADNS result if we have completed */
	adns_rrtype type; /**< the type we are looking for, only populated if we add to a queue */
	void *data;	/**< the User data based to the callback */
	char lookupdata[255]; /**< the look up data, only populated if we add to a queue */
	void (*callback) (void *data, adns_answer * a);
						      /**< a function pointer to call when we have a result */
	char mod_name[BUFSIZE];
};

struct DNSStats {
	int totalq;
	int maxqueued;
	int totalqueued;
	int success;
	int failure;
} DNSStats;


/** @brief DNS structures
  */
typedef struct dnslookup_struct DnsLookup;

/** @brief List of DNS queryies
 *  Contains DnsLookup entries 
 */
list_t *dnslist;

/** @brief list of DNS queries that are queued up
 * 
 */
list_t *dnsqueue;


void dns_check_queue();

/** @brief starts a DNS lookup
 *
 * starts a DNS lookup for str of type type can callback the function
 * when complete. Data is an identifier that is not modified to identify this lookup to the callback function
 *
 * @param str the record to lookup 
 * @param type The type of record to lookup. See adns.h for more details
 * @param callback the function to callback when we are complete
 * @param data a string to pass unmodified to the callback function to help identifing this lookup
 * 
 * @return returns 1 on success, 0 on failure (to add the lookup, not a successful lookup
*/

int
dns_lookup (char *str, adns_rrtype type, void (*callback) (void *data, adns_answer * a), void *data)
{
	lnode_t *dnsnode;
	DnsLookup *dnsdata;
	int status;
	struct sockaddr_in sa;

	SET_SEGV_LOCATION();

	dnsdata = malloc (sizeof (DnsLookup));
	DNSStats.totalq++;
	if (!dnsdata) {
		nlog (LOG_CRITICAL, LOG_CORE, "DNS: Out of Memory");
		DNSStats.failure++;
		return 0;
	}
	/* set the module name */
	/* This is a bad bad hack... */
	if (segv_inmodule[0]) {
		/* why MAXHOST? because thats the size of mod_name!?!? */
		strncpy(dnsdata->mod_name, segv_inmodule, BUFSIZE);
	} else {
		strncpy(dnsdata->mod_name, "NeoStats", BUFSIZE);
	}
	dnsdata->data = data;
	dnsdata->callback = callback;
	dnsdata->type = type;
	strncpy(dnsdata->lookupdata, str, 254);
	if (list_isfull (dnslist)) {
		nlog (LOG_DEBUG1, LOG_CORE, "DNS: Lookup list is full, adding to queue");
		dnsnode = lnode_create (dnsdata);
		list_append (dnsqueue, dnsnode);
		DNSStats.totalqueued++;
		if (list_count(dnsqueue) > DNSStats.maxqueued) {
			DNSStats.maxqueued = list_count(dnsqueue);
		}
		return NS_SUCCESS;
	}


	if (type == adns_r_ptr) {
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = inet_addr (str);
		status = adns_submit_reverse (ads, (const struct sockaddr *) &sa, type, adns_qf_owner | adns_qf_cname_loose, NULL, &dnsdata->q);
	} else {
		status = adns_submit (ads, str, type, adns_qf_owner | adns_qf_cname_loose, NULL, &dnsdata->q);
	}
	if (status) {
		nlog (LOG_WARNING, LOG_CORE, "DNS: adns_submit error: %s", strerror (status));
		free (dnsdata);
		DNSStats.failure++;
		return 0;
	}

	nlog (LOG_DEBUG1, LOG_CORE, "DNS: Added dns query %s to list", dnsdata->lookupdata);
	/* if we get here, then the submit was successful. Add it to the list of queryies */
	dnsnode = lnode_create (dnsdata);
	list_append (dnslist, dnsnode);

	return 1;
}


/** @brief sets up DNS subsystem
 *
 * configures ADNS for use with NeoStats.
 *
 * @return returns 1 on success, 0 on failure
*/

int
init_dns ()
{
	int adnsstart;

	SET_SEGV_LOCATION();
	dnslist = list_create (DNS_QUEUE_SIZE);
	if (!dnslist)
		return NS_FAILURE;
	/* dnsqueue is unlimited. */
	dnsqueue = list_create(-1);
	if (!dnsqueue) 
		return NS_FAILURE;
#ifndef DEBUG
	adnsstart = adns_init (&ads, adns_if_noerrprint | adns_if_noautosys, 0);
#else
	adnsstart = adns_init (&ads, adns_if_debug | adns_if_noautosys, 0);
#endif
	if (adnsstart) {
		printf ("ADNS init failed: %s\n", strerror (adnsstart));
		nlog (LOG_CRITICAL, LOG_CORE, "ADNS init failed: %s", strerror (adnsstart));
		return NS_FAILURE;
	}
	return NS_SUCCESS;

}

/* @brief Clean up ADNS data when we shutdown 
 *
 */
void 
fini_adns() {
	lnode_t *dnsnode;
	DnsLookup *dnsdata;

	SET_SEGV_LOCATION();

	dnsnode = list_first (dnslist);
	while (dnsnode) {
		dnsdata = lnode_get(dnsnode);
		adns_cancel(dnsdata->q);
		free (dnsdata->a);
		free (dnsdata);
	}
	list_destroy_nodes(dnslist);
	dnsnode = list_first(dnsqueue);
	while (dnsnode) {
		dnsdata = lnode_get(dnsnode);
		free(dnsdata);
	}
	list_destroy_nodes(dnsqueue);
	free(ads);
}
/** @brief Canx any DNS queries for modules we might be unloading
 * 
 * @param module name
 * @return Nothing
 */
void 
canx_dns(const char *modname) {
	lnode_t *dnsnode, *lnode2;
	DnsLookup *dnsdata;

	SET_SEGV_LOCATION();
	
	dnsnode = list_first (dnslist);
	while (dnsnode) {
		dnsdata = lnode_get(dnsnode);
		if (!strcasecmp(dnsdata->mod_name, modname)) {
			adns_cancel(dnsdata->q);
			free (dnsdata->a);
			free (dnsdata);
			lnode2 = list_next(dnslist, dnsnode);
			list_delete(dnslist, dnsnode);
			lnode_destroy(dnsnode);
			dnsnode = lnode2;
		}
	}
        dnsnode = list_first(dnsqueue);
        while (dnsnode) {
                dnsdata = lnode_get(dnsnode);
                if (!strcasecmp(dnsdata->mod_name, modname)) {
                        free(dnsdata);
                        lnode2 = list_next(dnsqueue, dnsnode);
                        list_delete(dnsqueue, dnsnode);
                        lnode_destroy(dnsnode);
                        dnsnode = lnode2;
                }
        }
        dns_check_queue();
}



/** @brief Checks for Completed DNS queries
 *
 *  Goes through the dnslist of pending queries and calls the callback function for each lookup
 *  with the adns_answer set. Always calls the callback function even if the lookup was unsuccessful
*  its upto the callback function to make check the answer struct to see if it failed or not
 *
 * @return Nothing
*/

void
do_dns ()
{
	lnode_t *dnsnode, *dnsnode1;
	int status;
	DnsLookup *dnsdata;

	SET_SEGV_LOCATION();
	/* if the list is empty, no use doing anything */
	if (list_isempty (dnslist)) {
		dns_check_queue();
		return;
	}

	dnsnode = list_first (dnslist);
	while (dnsnode) {
		/* loop through the list */
		dnsdata = lnode_get (dnsnode);
		status = adns_check (ads, &dnsdata->q, &dnsdata->a, NULL);
		/* if status == eagain, the lookup hasn't completed yet */
		if (status == EAGAIN) {
			nlog (LOG_DEBUG2, LOG_CORE, "DNS: Lookup hasn't completed for %s",dnsdata->lookupdata);
			dnsnode = list_next (dnslist, dnsnode);
			break;
		}
		/* there was an error */
		if (status) {
			nlog (LOG_CRITICAL, LOG_CORE, "DNS: Baaaad error on adns_check: %s. Please report to NeoStats Group", strerror (status));
			DNSStats.failure++;
			
			/* set this so nlog works good */
			SET_SEGV_INMODULE(dnsdata->mod_name);
			/* call the callback function with answer set to NULL */
			dnsdata->callback (dnsdata->data, NULL);
			CLEAR_SEGV_INMODULE();
			/* delete from list */
			dnsnode1 = list_delete (dnslist, dnsnode);
			dnsnode = list_next (dnslist, dnsnode1);
			free (dnsdata->a);
			free (dnsdata);
			lnode_destroy (dnsnode1);
			break;
		}
		nlog (LOG_DEBUG1, LOG_CORE, "DNS: Calling callback function with data %s for module %s", dnsdata->lookupdata, dnsdata->mod_name);
		DNSStats.success++;
		SET_SEGV_INMODULE(dnsdata->mod_name);
		/* call the callback function */
		dnsdata->callback (dnsdata->data, dnsdata->a);
		CLEAR_SEGV_INMODULE();
		/* delete from list */
		dnsnode1 = list_delete (dnslist, dnsnode);
		dnsnode = list_next (dnslist, dnsnode1);
		free (dnsdata->a);
		free (dnsdata);
		lnode_destroy (dnsnode1);
	}
	dns_check_queue();
}
/** @breif Checks the DNS queue and if we can
 * add new queries to the active DNS queries and remove from Queue 
*/
void dns_check_queue() {
	lnode_t *dnsnode, *dnsnode2;
	DnsLookup *dnsdata;
	struct sockaddr_in sa;
	int status;
	
	/* first, if the DNSLIST is full, just exit straight away */
	if (list_isfull(dnslist)) {
		nlog(LOG_DEBUG2, LOG_CORE, "DNS list is still full. Can't work on queue");
		return;
	}
	/* if the dnsqueue isn't empty, then lets process some more till we are full again */
	if (!list_isempty(dnsqueue)) {
		dnsnode = list_first(dnsqueue);
		while ((dnsnode) && (!list_isfull(dnslist))) {
			dnsdata = lnode_get(dnsnode);	
			nlog(LOG_DEBUG2, LOG_CORE, "Moving DNS query %s from queue to active", dnsdata->lookupdata);
			if (dnsdata->type == adns_r_ptr) {
				sa.sin_family = AF_INET;
				sa.sin_addr.s_addr = inet_addr (dnsdata->lookupdata);
				status = adns_submit_reverse (ads, (const struct sockaddr *) &sa, dnsdata->type, adns_qf_owner | adns_qf_cname_loose, NULL, &dnsdata->q);
			} else {
				status = adns_submit (ads, dnsdata->lookupdata, dnsdata->type, adns_qf_owner | adns_qf_cname_loose, NULL, &dnsdata->q);
			}
			if (status) {
				/* delete from queue and delete node */
				nlog (LOG_WARNING, LOG_CORE, "DNS: adns_submit error: %s", strerror (status));
				free (dnsdata);
				dnsnode2 = dnsnode;
				dnsnode = list_next(dnsqueue, dnsnode);
				list_delete(dnsqueue, dnsnode2);
				lnode_destroy(dnsnode2);
				continue;
			}
			/* move from queue to active list */
			dnsnode2 = dnsnode;
			dnsnode = list_next(dnsqueue, dnsnode);
			list_delete(dnsqueue, dnsnode2);
			list_append(dnslist, dnsnode2);
			nlog (LOG_DEBUG1, LOG_CORE, "DNS: Added dns query %s to list", dnsdata->lookupdata);
		/* while loop */
		}
	/* isempty */
	}
}


void do_dns_stats_Z() {
#if 0
	numeric (RPL_MEMSTATS, u->nick, "Active DNS queries: %d", (int) list_count(dnslist));
	numeric (RPL_MEMSTATS, u->nick, "Queued DNS Queries: %d", (int) list_count(dnsqueue));
	numeric (RPL_MEMSTATS, u->nick, "Max Queued Queries: %d", DNSStats.maxqueued);
	numeric (RPL_MEMSTATS, u->nick, "Total DNS Questions: %d", DNSStats.totalq);
	numeric (RPL_MEMSTATS, u->nick, "SuccessFull Lookups: %d", DNSStats.success);
	numeric (RPL_MEMSTATS, u->nick, "Un-Successfull Lookups: %d", DNSStats.failure);
#endif
}
