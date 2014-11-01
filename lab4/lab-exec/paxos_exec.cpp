// Basic routines for Paxos implementation

#include "make_unique.h"
#include "paxmsg.h"
#include "paxserver.h"
#include "log.h"
#include "paxlog.h"

std::unordered_map<viewstamp_t, int> msgAckd;

void paxserver::execute_arg(const struct execute_arg& ex_arg) {
  if (primary()) {
    if (paxlog.find_rid(ex_arg.nid, ex_arg.rid) &&
        vc_state.view.vid == ex_arg.vid) {
      net->drop(this, ex_arg, "Duplicate Execute Message");
    } else {

      // asign the viewstamp
      viewstamp_t vs;
      vs.vid = vc_state.view.vid;
      ts = ts + 1;
      vs.ts = ts;

      // log the request
      paxlog.log(ex_arg.nid, ex_arg.rid, vs, ex_arg.request,
          get_serv_cnt(vc_state.view), net->now());

      // send the replicate request to all other replicas
      std::set<node_id_t> servers = get_other_servers(vc_state.view);
      for (node_id_t node_id : servers) {
        auto new_repl_arg = std::make_unique<struct replicate_arg>(
            vs, ex_arg, vc_state.latest_seen);
        net->send(this, node_id, std::move(new_repl_arg));
      }
    }
  } else {
    // send the fail message so that client can reset the primary
    auto ex_fail = std::make_unique<struct execute_fail>(vc_state.view.vid,
        vc_state.view.primary, ex_arg.rid);
    net->send(this, ex_arg.nid, std::move(ex_fail));
  }
}

void paxserver::replicate_arg(const struct replicate_arg& repl_arg) {
  auto ex_arg = repl_arg.arg;
  if (paxlog.find_rid(ex_arg.nid, ex_arg.rid) &&
      vc_state.view.vid == ex_arg.vid) {
    net->drop(this, ex_arg, "Duplicate Replicate Message");
  } else {

    // log the request
    paxlog.log(ex_arg.nid, ex_arg.rid, repl_arg.vs, ex_arg.request,
        get_serv_cnt(vc_state.view), net->now());

    // TODO(goyalankit) Send ack only when all the vs less than
    // the received has been logged.
    // send ack to the primary
    auto repl_res = std::make_unique<struct replicate_res>(repl_arg.vs);
    net->send(this, vc_state.view.primary, std::move(repl_res));
  }
  //MASSERT(0, "replicate_arg not implemented\n");
}

void paxserver::replicate_res(const struct replicate_res& repl_res) {

  viewstamp_t vs = repl_res.vs;
  // increment the ack received
  if (msgAckd[vs] == 0) {
    (msgAckd[vs]) = 2; // 2 includes increment for self also
  } else {
    msgAckd[repl_res.vs] += 1;
  }

  // check if majority of acks received
  int majority = get_serv_cnt(vc_state.view) / 2 + 1;
  if (msgAckd[repl_res.vs] >= majority) {
    LOG(l::DEBUG, "Log: " << "Majority of Acks received\n");
    // execute!

  }
}

void paxserver::accept_arg(const struct accept_arg& acc_arg) {
  MASSERT(0, "accept_arg not implemented\n");
}
