// Basic routines for Paxos implementation

#include "make_unique.h"
#include "paxmsg.h"
#include "paxserver.h"
#include "log.h"
#include "paxlog.h"


bool myTrimf(const std::unique_ptr<Paxlog::tup>& mytup) {
  const Paxlog::tup *tup = mytup.get();
  return tup->executed;
}

void paxserver::execute_arg(const struct execute_arg& ex_arg) {
  if (primary()) {
    if (paxlog.find_rid(ex_arg.nid, ex_arg.rid) &&
        vc_state.view.vid == ex_arg.vid) {
      net->drop(this, ex_arg, "Duplicate Execute Message");
    } else {

      // asign the viewstamp
      viewstamp_t vs;
      vs.vid = vc_state.view.vid;
      vs.ts = ts;
      ts = ts + 1;

      // log the request
      paxlog.log(ex_arg.nid, ex_arg.rid, vs, ex_arg.request,
          get_serv_cnt(vc_state.view), net->now());

      // send the replicate request to all other replicas
      std::set<node_id_t> servers = get_other_servers(vc_state.view);
      for (node_id_t node_id : servers) {
        auto new_repl_arg = std::make_unique<struct replicate_arg>(
            vs, ex_arg, vc_state.latest_seen);
        send_msg(node_id, std::move(new_repl_arg));
      }
    }
  } else {
    // send the fail message so that client can reset the primary
    auto ex_fail = std::make_unique<struct execute_fail>(vc_state.view.vid,
        vc_state.view.primary, ex_arg.rid);
    send_msg(ex_arg.nid, std::move(ex_fail));
  }
}

void paxserver::replicate_arg(const struct replicate_arg& repl_arg) {
  auto ex_arg = repl_arg.arg;
  if (paxlog.find_rid(ex_arg.nid, ex_arg.rid) &&
      vc_state.view.vid == ex_arg.vid) {
    net->drop(this, ex_arg, "Duplicate Replicate Message");
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
        // since messages can be out of order
        // we want to keep this value updated.
        if (paxlog.latest_accept() < committed)
          paxlog.set_latest_accept(committed);
      }
    }

    paxlog.trim_front(myTrimf);
    // log the request
    paxlog.log(ex_arg.nid, ex_arg.rid, repl_arg.vs, ex_arg.request,
        get_serv_cnt(vc_state.view), net->now());
    auto repl_res = std::make_unique<struct replicate_res>(repl_arg.vs);
    send_msg(vc_state.view.primary, std::move(repl_res));
  }
}



void paxserver::replicate_res(const struct replicate_res& repl_res) {

  // increment number of responses received
  paxlog.incr_resp(repl_res.vs);

  bool execute = true;
  // check if majority of acks received
  for (auto it = paxlog.begin(), ie = paxlog.end(); it != ie; ++it) {
#if 0
      std::cout << "------------------------------" << std::endl;
      std::cout << (*it)->vs << " || " << paxlog.latest_exec() << std::endl;
      std::cout << "------------------------------" << std::endl;
#endif
      if (paxlog.next_to_exec(it) && ((*it)->resp_cnt) > 2) {
        std::string result = paxop_on_paxobj(*it);
        paxlog.set_latest_accept((*it)->vs);
        vc_state.latest_seen = (*it)->vs;
        paxlog.execute(*it);

        auto ex_success = std::make_unique<struct execute_success>(result, (*it)->rid);
        send_msg((*it)->src, std::move(ex_success));
      }
      execute &= (*it)->executed;
  }
  paxlog.trim_front(myTrimf);

  if (execute){
    // send the replicate request to all other replicas
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
  send_msg(vc_state.view.primary, std::make_unique<nop_msg>());
  paxlog.trim_front(myTrimf);
}
