#ifndef __DHASH_IMPL_H__
#define __DHASH_IMPL_H__

#include "dhash.h"
#include <qhash.h>

// Forward declarations.
class RPC_delay_args;

class dhashcli;
struct dbrec;
class dbfe;
class dhblock_srv;

class location;
struct hashID;

// Callback typedefs
typedef callback<void,bool,dhash_stat>::ptr cbstore;

// Helper structs/classes
struct store_chunk {
  store_chunk *next;
  unsigned int start;
  unsigned int end;

  store_chunk (unsigned int s, unsigned int e, store_chunk *n) : next(n), start(s), end(e) {};
  ~store_chunk () {};
};

struct store_state {
  chordID key;
  unsigned int size;
  store_chunk *have;
  char *buf;

  ihash_entry <store_state> link;
   
  store_state (chordID k, unsigned int z) : key(k), 
    size(z), have(0), buf(New char[z]) { };

  ~store_state ()
  { 
    delete[] buf; 
    store_chunk *cnext;
    for (store_chunk *c=have; c; c=cnext) {
      cnext = c->next;
      delete c;
    }
  };
  bool gap ();
  bool addchunk (unsigned int start, unsigned int end, void *base);
  bool iscomplete ();
};

struct pk_partial {
  ptr<dbrec> val;
  int bytes_read;
  int cookie;
  ihash_entry <pk_partial> link;

  pk_partial (ptr<dbrec> v, int c) : val (v), 
		bytes_read (0),
		cookie (c) {};
};

class dhash_impl : public dhash {
  int pk_partial_cookie;
  
  qhash<dhash_ctype, ref<dhblock_srv> > blocksrv;

  ptr<vnode> host_node;
  ptr<dhashcli> cli;

  ihash<chordID, store_state, &store_state::key, 
    &store_state::link, hashID> pst;
  
  ihash<int, pk_partial, &pk_partial::cookie, 
    &pk_partial::link> pk_cache;
  
  void route_upcall (int procno, void *args, cbupcalldone_t cb);

  void doRPC (ptr<location> n, const rpc_program &prog, int procno,
	      ptr<void> in, void *out, aclnt_cb cb, 
	      cbtmo_t cb_tmo = NULL);
  void doRPC (const chord_node &n, const rpc_program &prog, int procno,
	      ptr<void> in, void *out, aclnt_cb cb,
	      cbtmo_t cb_tmo = NULL);
  void doRPC (const chord_node_wire &n, const rpc_program &prog, int procno,
	      ptr<void> in, void *out, aclnt_cb cb,
	      cbtmo_t cb_tmo = NULL);
  void doRPC_reply (svccb *sbp, void *res, 
		    const rpc_program &prog, int procno);
  void dispatch (user_args *a);

  void storesvc_cb (user_args *sbp, s_dhash_insertarg *arg, 
		    bool already_present, dhash_stat err);
  dhash_fetchiter_res * block_to_res (dhash_stat err, s_dhash_fetch_arg *arg,
				      int cookie, ptr<dbrec> val);
  void fetchiter_gotdata_cb (cbupcalldone_t cb, s_dhash_fetch_arg *farg,
			     int cookie, ptr<dbrec> val, dhash_stat stat);
  void fetchiter_sbp_gotdata_cb (user_args *sbp, s_dhash_fetch_arg *farg,
				 int cookie, ptr<dbrec> val, dhash_stat stat);
  void sent_block_cb (dhash_stat *s, clnt_stat err);

  void fetch (blockID id, int cookie, cbvalue cb);

  void store (s_dhash_insertarg *arg, cbstore cb);
  
  bool key_present (const blockID &n);
  ptr<dbrec> dblookup (const blockID &i);

  void dofetchrec (user_args *sbp, dhash_fetchrec_arg *arg);
  void dofetchrec_nexthop (user_args *sbp, dhash_fetchrec_arg *arg,
			   ptr<location> p);
  void dofetchrec_nexthop_cb (user_args *sbp, dhash_fetchrec_arg *arg,
			      ptr<dhash_fetchrec_res> res,
			      timespec t,
			      clnt_stat err);
  void dofetchrec_local (user_args *sbp, dhash_fetchrec_arg *arg);
  void dofetchrec_assembler (user_args *sbp, dhash_fetchrec_arg *arg,
			     vec<ptr<location> > succs);
  void dofetchrec_assembler_cb (user_args *sbp, dhash_fetchrec_arg *arg,
				dhash_stat s, ptr<dhash_block> b, route r);

  void merkle_dispatch (user_args *sbp);
  /* statistics */
  long bytes_stored;
  long keys_stored;
  long keys_replicated;
  long keys_cached;
  long keys_others;
  long bytes_served;
  long keys_served;
  long rpc_answered;

 public:
  dhash_impl (ptr<vnode> v, str dbname);
  ~dhash_impl ();

  vec<dstat> stats ();
  strbuf key_info ();
  void print_stats ();
  
  void stop ();
  void start (bool randomize = false);
};

#endif
