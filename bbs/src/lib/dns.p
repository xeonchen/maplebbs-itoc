/* dns.c */
void dns_init(void);
/* dns_aton.c */
unsigned long dns_aton(char *name);
int dns_query(char *name, int qtype, querybuf *ans);
/* dns_ident.c */
void dns_ident(int sock, struct sockaddr_in *from, char *rhost, char *ruser);
/* dns_name.c */
int dns_name(unsigned char *addr, char *name);
/* dns_open.c */
int dns_open(char *host, int port);
/* dns_smtp.c */
int dns_mx(char *domain, char *mxlist);
int dns_smtp(char *host);
