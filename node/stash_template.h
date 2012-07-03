#ifndef STASHTEMPLATE_H
#define STASHTEMPLATE_H

#include <node.h>
#include "mustache.h"

class StashTemplate : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> target);

 private:
  StashTemplate();
  ~StashTemplate();

  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  static v8::Handle<v8::Value> Render(const v8::Arguments& args);
  mustache_node_t* ast;
};

#endif
