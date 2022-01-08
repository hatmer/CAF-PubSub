/******************************************************************************\
 * Illustrates semantics of request().{then|await|receive}.                   *
\******************************************************************************/

#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

#include "caf/all.hpp"

using std::endl;
using std::vector;
using std::chrono::seconds;
using namespace caf;

// --(rst-topic-begin)--
using topic
  = typed_actor<result<void>(put_atom, int32_t), // 'put' writes to the topic
                result<int32_t>(get_atom)>;      // 'get 'reads from the topic

struct topic_state {
  static constexpr inline const char* name = "topic";

  topic::pointer self;

  int32_t value;

  topic_state(topic::pointer ptr, int32_t val) : self(ptr), value(val) {
    // nop
  }

  topic_state(const topic_state&) = delete;

  topic_state& operator=(const topic_state&) = delete;

  topic::behavior_type make_behavior() {
    return {
      [=](put_atom, int32_t val) { value = val; },
      [=](get_atom) { return value; },
    };
  }
};

using topic_impl = topic::stateful_impl<topic_state>;
// --(rst-ddtopic-end)--



// blocking functions that call receive on each topic in vector: correspond to publisher and subscriber

void publisher(blocking_actor* self, vector<topic> topics) {
  for (auto& x : topics)
    self->request(x, seconds(1), put_atom_v, 5);
}

void subscriber(blocking_actor* self, vector<topic> topics) {
  for (auto& x : topics)
    self->request(x, seconds(1), get_atom_v)
      .receive(
        [&](int32_t y) {
          aout(self) << "topic #" << x.id() << " -> " << y << endl;
        },
        [&](error& err) {
          aout(self) << "topic #" << x.id() << " -> " << to_string(err) << endl;
        });    
}


// testing
void caf_main(actor_system& system) {
  // vector of 5 different topics
  vector<topic> topics;
  for (int32_t i = 0; i < 5; ++i)
    topics.emplace_back(system.spawn<topic_impl>(i));
  scoped_actor self{system};

  aout(self) << "publisher" << endl;
  system.spawn(publisher, topics);

  aout(self) << "subscriber" << endl;
  system.spawn(subscriber, topics);
}
// --(rst-main-end)--

CAF_MAIN()
