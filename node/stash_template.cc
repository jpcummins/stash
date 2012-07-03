#define BUILDING_NODE_EXTENSION
#include <node.h>
#include <string.h>
#include "stash_template.h"

using namespace v8;

StashTemplate::StashTemplate() : ast(NULL) {};
StashTemplate::~StashTemplate() {};

void StashTemplate::Init(Handle<Object> target) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("Template"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  // Prototype
  tpl->PrototypeTemplate()->Set(String::NewSymbol("render"),
      FunctionTemplate::New(Render)->GetFunction());

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("Template"), constructor);
}

Handle<Value> StashTemplate::New(const Arguments& args) {
  HandleScope scope;

  StashTemplate* obj = new StashTemplate();
  v8::Local<v8::String> template_string = args[0]->ToString();

  char* c_template_string = (char*) malloc(template_string->Utf8Length() + 1);
  memset(c_template_string, '\0', template_string->Utf8Length() + 1);
  template_string->WriteUtf8(c_template_string);

  obj->ast = mustache_create_node();
  mustache_error_t error;
  mustache_error_t* pError = &error;
  mustache_build_template(c_template_string, obj->ast, &pError);

  obj->Wrap(args.This());
  return args.This();
}

Handle<Value> StashTemplate::Render(const Arguments& args) {
  HandleScope scope;
  // StashTemplate* obj = ObjectWrap::Unwrap<StashTemplate>(args.This());
  return scope.Close(Number::New(5));
}
