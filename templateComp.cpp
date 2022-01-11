#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>

#include "ftypename.hpp"

#include "caf/all.hpp"

using std::endl;
using std::vector;
using std::chrono::seconds;
//using namespace caf;

using std::queue;

template <class T>
using template_topic
  = caf::typed_actor<
								caf::result<void>(caf::put_atom, T),      // 'put' writes to the topic
								caf::result<T>(caf::get_atom, int32_t)>;   // 'get 'reads from the topic

template <class T>
struct topic_state {

	class
  template_topic<T>::pointer self;
};


//NO VERSION OF THIS WILL COMPILE
//Perhaps not possible..
template <class T>
using topic_impl = class template_topic<T>::template stateful_impl< topic_state< T> >;