// Basic routines for Paxos implementation

#include "make_unique.h"
#include "paxmsg.h"
#include "paxserver.h"
#include "log.h"
#include "paxlog.h"
#include <assert.h>


/*
 * My trim function: truncate if the tup has already
 * been executed.
 *
 * */
bool trim_fn(const std::unique_ptr<Paxlog::tup>& mytup) {
  const Paxlog::tup *tup = mytup.get();
  return tup->executed;
}
/*
 * 1. Check that client is not issuing request to non primary.
 * 2. Check that we have not already executed and truncated the log
 *    for the requested rid from client cid.
 * 3. Check if the request is already logged in.
 * 4. Update the viewstamp, log it and send message to all replicas.
 *
 * */
void paxserver::execute_arg(const struct execute_arg& ex_arg) {
  if (primary()) {
    // check if we have executed and truncated the log for this request
    for (auto it = exec_rid_cache.begin(), ie = exec_rid_cache.end();
        it != ie; ++it ){
      if (((*it).first == ex_arg.nid) && ((*it).second == ex_arg.rid)) {
        net->drop(this, ex_arg, "Duplicate Execute Message");
        return;
      }
    }
    // check if it is present in the log
    if (paxlog.find_rid(ex_arg.nid, ex_arg.rid) &&
        vc_state.view.vid == ex_arg.vid) {
      net->drop(this, ex_arg, "Duplicate Execute Message");
      return;
    } else { /* can't find the rid in logs and haven't executed it either */
      // asign the viewstamp
      viewstamp_t vs;
      vs.vid = vc_state.view.vid;
      vs.ts = ts;
      ts = ts + 1;
      // log the request
      paxlog.log(ex_arg.nid, ex_arg.rid, vs, ex_arg.request,
          get_serv_cnt(vc_state.view), net->now());
      // update the latest vs
      paxlog.set_latest_accept(vs);
      // send the replicate request to all other replicas
      std::set<node_id_t> servers = get_other_servers(vc_state.view);
      for (node_id_t node_id : servers) {
        auto new_repl_arg = std::make_unique<struct replicate_arg>(
            vs, ex_arg, vc_state.latest_seen);
        send_msg(node_id, std::move(new_repl_arg));
      }
    }
  } else { /* not a primary */
    // send the fail message so that client can reset the primary
    auto ex_fail = std::make_unique<struct execute_fail>(vc_state.view.vid,
        vc_state.view.primary, ex_arg.rid);
    send_msg(ex_arg.nid, std::move(ex_fail));
  }
}

/*
 * Executed by replicas to log the new request and execute the previous
 * enteries.
 *
 * */
void paxserver::replicate_arg(const struct replicate_arg& repl_arg) {
  auto ex_arg = repl_arg.arg;
  // check for duplicate messages
  if (paxlog.find_rid(ex_arg.nid, ex_arg.rid) &&
      vc_state.view.vid == ex_arg.vid) {
    net->drop(this, ex_arg, "Duplicate Replicate Message");
    return;
  } else {
    viewstamp_t committed = repl_arg.committed;
    viewstamp_t latest_exec = paxlog.latest_exec();
    // execute all the remaining ts
    // Replica Executing here
    for (auto it = paxlog.begin(), ie = paxlog.end(); it != ie; ++it) {
      if (paxlog.next_to_exec(it) && (latest_exec <= committed
            || latest_exec <= paxlog.latest_accept())) {
        // we don't need to return this from replicas
        std::string result = paxop_on_paxobj(*it);
        paxlog.execute(*it);
        // update the committed timestamp.
        vc_state.latest_seen.ts += 1;
        it = paxlog.begin();
        // since messages can be out of order
        // we want to keep this value updated.
        if (paxlog.latest_accept() < committed)
          paxlog.set_latest_accept(committed);
      }
    }
    // trim the front
    paxlog.trim_front(trim_fn);
    // log the request at replica
    paxlog.log(ex_arg.nid, ex_arg.rid, repl_arg.vs, ex_arg.request,
        get_serv_cnt(vc_state.view), net->now());
    auto repl_res = std::make_unique<struct replicate_res>(repl_arg.vs);
    send_msg(vc_state.view.primary, std::move(repl_res));
  }
}



void paxserver::replicate_res(const struct replicate_res& repl_res) {
  // increment number of responses received
  // if this fails, it means that it couldn't find the
  // viewstamp. receive of replicate_res implies that the request
  // was logged. So the entry has been executed and log has been trimmed
  if (!paxlog.incr_resp(repl_res.vs)) {
    net->drop(this, repl_res, "Duplicate Replicate Res Message");
  }
  unsigned majority = get_serv_cnt(vc_state.view) / 2 + 1;
  // check if majority of acks received
  for (auto it = paxlog.begin(), ie = paxlog.end(); it != ie; ++it) {
#if 0
    std::cout << "------------------------------" << std::endl;
    std::cout << (*it)->vs << " || " << paxlog.latest_exec() << std::endl;
    std::cout << "------------------------------" << std::endl;
#endif
    if (paxlog.next_to_exec(it) && ((*it)->resp_cnt) > majority) {
      std::string result = paxop_on_paxobj(*it);
      paxlog.set_latest_accept((*it)->vs);
      vc_state.latest_seen = (*it)->vs;
      paxlog.execute(*it);
      auto ex_success = std::make_unique<struct execute_success>(result, (*it)->rid);
      send_msg((*it)->src, std::move(ex_success));
    }
  }
  paxlog.trim_front(trim_fn);

  // if you have no entries you send accept message to replicas
  if (paxlog.empty()) {
    std::set<node_id_t> servers = get_other_servers(vc_state.view);
    for (node_id_t node_id : servers) {
      auto acc_arg = std::make_unique<struct accept_arg>(vc_state.latest_seen);
      send_msg(node_id, std::move(acc_arg));
    }
  }
}

void paxserver::accept_arg(const struct accept_arg& acc_arg) {
  viewstamp_t committed = acc_arg.committed;
  viewstamp_t latest_exec = paxlog.latest_exec();
  // execute all the remaining ts
  // Replica Executing here
  for (auto it = paxlog.begin(), ie = paxlog.end(); it != ie; ++it) {
    if (paxlog.next_to_exec(it) && (latest_exec <= committed
          || latest_exec <= paxlog.latest_accept())) {
      // we don't need to return this from replicas
      std::string result = paxop_on_paxobj(*it);
      paxlog.execute(*it);
      // since messages can be out of order
      // we want to keep this value updated.
      if (paxlog.latest_accept() < committed)
        paxlog.set_latest_accept(committed);
    }
  }
  paxlog.trim_front(trim_fn);
}
