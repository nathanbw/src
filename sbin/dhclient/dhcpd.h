/*	$OpenBSD: dhcpd.h,v 1.202 2017/07/02 09:11:13 krw Exp $	*/

/*
 * Copyright (c) 2004 Henning Brauer <henning@openbsd.org>
 * Copyright (c) 1995, 1996, 1997, 1998, 1999
 * The Internet Software Consortium.    All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Ted Lemon <mellon@fugue.com> in cooperation with Vixie
 * Enterprises.  To learn more about the Internet Software Consortium,
 * see ``http://www.vix.com/isc''.  To learn more about Vixie
 * Enterprises, see ``http://www.vix.com''.
 */

#define	LOCAL_PORT	68
#define	REMOTE_PORT	67
#define	INTERNALSIG	INT_MAX
#define DB_TIMEFMT	"%w %Y/%m/%d %T UTC"

struct option {
	char *name;
	char *format;
};

struct option_data {
	unsigned int	 len;
	u_int8_t	*data;
};

struct reject_elem {
	TAILQ_ENTRY(reject_elem) next;
	struct in_addr		 addr;
};

struct client_lease {
	TAILQ_ENTRY(client_lease) next;
	time_t			 expiry, renewal, rebind;
	struct in_addr		 address;
	struct in_addr		 next_server;
	char			*server_name;
	char			*filename;
	char			*resolv_conf;
	char			 ssid[32];
	uint8_t			 ssid_len;
	unsigned int		 is_static;
	struct option_data	 options[256];
};
#define BOOTP_LEASE(l)	((l)->options[DHO_DHCP_MESSAGE_TYPE].len == 0)

/* Possible states in which the client can be. */
enum dhcp_state {
	S_PREBOOT,
	S_REBOOTING,
	S_INIT,
	S_SELECTING,
	S_REQUESTING,
	S_BOUND,
	S_RENEWING,
	S_REBINDING
};

struct client_config {
	struct option_data	defaults[256];
	enum {
		ACTION_DEFAULT,
		ACTION_SUPERSEDE,
		ACTION_PREPEND,
		ACTION_APPEND
	} default_actions[256];

	struct in_addr		 address;
	struct in_addr		 next_server;
	struct option_data	 send_options[256];
	u_int8_t		 required_options[256];
	u_int8_t		 requested_options[256];
	u_int8_t		 ignored_options[256];
	int			 requested_option_count;
	int			 required_option_count;
	int			 ignored_option_count;
	time_t			 timeout;
	time_t			 initial_interval;
	time_t			 link_timeout;
	time_t			 retry_interval;
	time_t			 select_interval;
	time_t			 reboot_timeout;
	time_t			 backoff_cutoff;
	TAILQ_HEAD(, reject_elem) reject_list;
	char			*resolv_tail;
	char			*filename;
	char			*server_name;
};


struct interface_info {
	struct ether_addr	 hw_address;
	char			 name[IFNAMSIZ];
	char			 ssid[32];
	uint8_t			 ssid_len;
	int			 bfdesc; /* bpf - reading & broadcast writing*/
	int			 ufdesc; /* udp - unicast writing */
	unsigned char		*rbuf;
	size_t			 rbuf_max;
	size_t			 rbuf_offset;
	size_t			 rbuf_len;
	int			 errors;
	u_int16_t		 index;
	int			 linkstat;
	int			 rdomain;
	int			 flags;
#define	IFI_VALID_LLADDR	0x01
#define IFI_IS_RESPONSIBLE	0x08
#define IFI_IN_CHARGE		0x10
	struct dhcp_packet	 recv_packet;
	struct dhcp_packet	 sent_packet;
	int			 sent_packet_length;
	u_int32_t		 xid;
	time_t			 timeout;
	void			(*timeout_func)(struct interface_info *);
	u_int16_t		 secs;
	time_t			 first_sending;
	time_t			 startup_time;
	enum dhcp_state		 state;
	struct in_addr		 destination;
	time_t			 interval;
	struct in_addr		 requested_address;
	struct client_lease	*active;
	struct client_lease	*offer;
	TAILQ_HEAD(_leases, client_lease) leases;
};

#define	_PATH_DHCLIENT_CONF	"/etc/dhclient.conf"
#define	_PATH_DHCLIENT_DB	"/var/db/dhclient.leases"

/* External definitions. */

extern struct client_config *config;
extern struct imsgbuf *unpriv_ibuf;
extern volatile sig_atomic_t quit;
extern struct in_addr deleting;
extern struct in_addr adding;

/* options.c */
int cons_options(struct interface_info *, struct option_data *);
char *pretty_print_option(unsigned int, struct option_data *, int);
char *pretty_print_domain_search(unsigned char *, size_t);
char *pretty_print_string(unsigned char *, size_t, int);
char *pretty_print_classless_routes(unsigned char *, size_t);
void do_packet(struct interface_info *, unsigned int, struct in_addr,
    struct ether_addr *);

/* conflex.c */
extern int lexline, lexchar;
extern char *token_line, *tlname;
void new_parse(char *);
int next_token(char **, FILE *);
int peek_token(char **, FILE *);

/* parse.c */
void skip_to_semi(FILE *);
int parse_semi(FILE *);
char *parse_string(FILE *, unsigned int *);
int parse_ip_addr(FILE *, struct in_addr *);
int parse_cidr(FILE *, unsigned char *);
void parse_lease_time(FILE *, time_t *);
int parse_decimal(FILE *, unsigned char *, char);
int parse_hex(FILE *, unsigned char *);
time_t parse_date(FILE *);
void parse_warn(char *);

/* bpf.c */
void if_register_send(struct interface_info *);
void if_register_receive(struct interface_info *);
ssize_t send_packet(struct interface_info *, struct in_addr, struct in_addr);
ssize_t receive_packet(struct interface_info *, struct sockaddr_in *,
    struct ether_addr *);

/* dispatch.c */
void dispatch(struct interface_info *, int);
void set_timeout( struct interface_info *, time_t,
    void (*)(struct interface_info *));
void cancel_timeout(struct interface_info *);
void interface_link_forceup(char *, int);
int interface_status(char *);
void get_hw_address(struct interface_info *);
void sendhup(void);

/* tables.c */
extern const struct option dhcp_options[256];

/* dhclient.c */
extern char *path_dhclient_conf;
extern char *path_dhclient_db;

void dhcpoffer(struct interface_info *, struct option_data *, char *);
void dhcpack(struct interface_info *, struct option_data *,char *);
void dhcpnak(struct interface_info *, struct option_data *,char *);

void free_client_lease(struct client_lease *);

void routehandler(struct interface_info *, int);

/* packet.c */
void assemble_eh_header(struct interface_info *, struct ether_header *);
ssize_t decode_hw_header(unsigned char *, u_int32_t, struct ether_addr *);
ssize_t decode_udp_ip_header(unsigned char *, u_int32_t, struct sockaddr_in *);
u_int32_t checksum(unsigned char *, u_int32_t, u_int32_t);
u_int32_t wrapsum(u_int32_t);

/* clparse.c */
void read_client_conf(struct interface_info *);
void read_client_leases(struct interface_info *);

/* kroute.c */
void delete_addresses(char *);
void delete_address(struct in_addr);

void set_interface_mtu(int);
void add_address(struct in_addr, struct in_addr);

void flush_routes(void);

void add_route(struct in_addr, struct in_addr, struct in_addr, struct in_addr,
    int, int);

void flush_unpriv_ibuf(const char *);

int resolv_conf_priority(int, int);
