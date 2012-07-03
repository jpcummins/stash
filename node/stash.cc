#define BUILDING_NODE_EXTENSION
#include <node.h>
#include "stash_template.h"

using namespace v8;

void InitAll(Handle<Object> target) {
  StashTemplate::Init(target);
}

NODE_MODULE(stash, InitAll)
