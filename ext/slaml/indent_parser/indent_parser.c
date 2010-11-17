#include <ruby.h>
#include <stdio.h>

typedef enum {
  Prefix, Content, Suffix
} IndentParserStatus;

typedef struct {
  int prefix_length;
  VALUE children;
} IndentLevel;

typedef struct {
  int strip, track_empty_lines, prefix_length, content_length, suffix_length,
    stack_size, stack_limit, empty_lines, last_prefix;
  char *start, *content_start, *current;
  IndentParserStatus status;
  IndentLevel *stack;
  VALUE root;
} IndentParser;

static VALUE sOptions, sStrip, sMulti, sIndent, sTrackEmptyLines;

static VALUE indent_parser_initialize(int argc, VALUE *argv, VALUE self)
{
  if(argc > 0) {
    rb_ivar_set(self, sOptions, argv[0]);
  } else {
    rb_ivar_set(self, sOptions, rb_hash_new());
  }
  return Qnil;
}

inline static VALUE IndentParser_get_option(VALUE self, VALUE key)
{
  VALUE options = rb_ivar_get(self, sOptions);
  options = rb_convert_type(options, T_HASH, "Hash", "to_hash");
  if(rb_hash_aref(options, key) == Qfalse) {
    return Qfalse;
  } else {
    return Qtrue;
  }
}

static VALUE indent_parser_strip(VALUE self)
{
  return IndentParser_get_option(self, sStrip);
}

static VALUE indent_parser_track_empty_lines(VALUE self)
{
  return IndentParser_get_option(self, sTrackEmptyLines);
}

static VALUE indent_parser_empty_lines(VALUE self)
{
  VALUE options = rb_ivar_get(self, sOptions);
  options = rb_convert_type(options, T_HASH, "Hash", "to_hash");
  if(rb_hash_aref(options, sStrip) == Qfalse) {
    return Qfalse;
  } else {
    return Qtrue;
  }
}

inline void IndentParser_check_size(IndentParser *self)
{
  if(self->stack_size >= self->stack_limit-1) {
    if(self->stack_limit) {
      self->stack_limit = self->stack_limit*2;
    } else {
      self->stack_limit = 100;
    }
    self->stack = xrealloc(self->stack, self->stack_limit);
  }
}

inline void IndentParser_add_level(IndentParser *self, VALUE level)
{
  if(self->stack_size == 0 && level != self->root) {
    self->stack_size = 1;
  }
  IndentParser_check_size(self);
  IndentLevel entry = {self->prefix_length, level};
  self->stack[self->stack_size] = entry;
  self->stack_size++;
}

inline static VALUE IndentParser_detect_parent(IndentParser *self)
{
  if(self->stack_size <= 1) {
    return self->root;
  } else if(self->prefix_length == self->last_prefix) {
    self->stack_size--;
  } else if(self->prefix_length < self->last_prefix) {
    int i;
    for(i = 0; i < self->stack_size; i++) {      
      if(self->stack[i].prefix_length >= self->prefix_length) {
        self->stack_size = i;
        break;
      }
    }
  }
  return self->stack[self->stack_size < 1 ? 0 : self->stack_size - 1].children;
}

inline void IndentParser_newline(IndentParser *self)
{
  self->last_prefix = self->prefix_length;
  self->prefix_length = self->suffix_length = self->content_length = 0;
  self->status = Prefix;
}

inline void IndentParser_Prefix_newline(IndentParser *self)
{
  if(self->track_empty_lines) { self->empty_lines++; }
  IndentParser_newline(self);
}

inline static VALUE IndentParser_new_element(IndentParser *self, char *start, int length)
{
  VALUE parent   = IndentParser_detect_parent(self);
  VALUE element  = rb_ary_new();
  VALUE children = rb_ary_new();
  rb_ary_push(element, sIndent);
  rb_ary_push(element, INT2FIX(self->prefix_length));
  rb_ary_push(element, rb_str_new(start, length));
  rb_ary_push(children, sMulti);
  rb_ary_push(element, children);
  rb_ary_push(parent, element);
  return children;
}

inline void IndentParser_Content_newline(IndentParser *self)
{
  if(self->empty_lines) {
    int i;
    for(i = 0; i < self->empty_lines; i++) { IndentParser_new_element(self, "", 0); }
    self->empty_lines = 0;
  }
  VALUE children = IndentParser_new_element(self, self->content_start, self->content_length);
  IndentParser_add_level(self, children);
  IndentParser_newline(self);
}

inline void IndentParser_Suffix_newline(IndentParser *self)
{
  IndentParser_Content_newline(self);
}

inline void IndentParser_Prefix_space(IndentParser *self)
{
  self->prefix_length++;
}

inline void IndentParser_Suffix_space(IndentParser *self)
{
  self->suffix_length++;
}

inline void IndentParser_Content_character(IndentParser *self)
{
  self->content_length++;
}

inline void IndentParser_Content_space(IndentParser *self)
{
  if(self->strip) {
    self->status = Suffix;
    IndentParser_Suffix_space(self);
  } else {
    IndentParser_Content_character(self);
  }
}

inline void IndentParser_Prefix_character(IndentParser *self)
{
  self->content_start = self->current;
  self->content_length = 0;
  self->status = Content;
  IndentParser_Content_character(self);
}

inline void IndentParser_Suffix_character(IndentParser *self)
{
  self->content_length = self->content_length + self->suffix_length;
  self->suffix_length  = 0;
  self->status         = Content;
  IndentParser_Content_character(self);
}

inline void IndentParser_process(IndentParser *self, char c)
{
  switch(c)
  {
    case '\r':
      break;
    case '\n':
      switch(self->status)
      {
        case Prefix:  IndentParser_Prefix_newline(self);    break;
        case Content: IndentParser_Content_newline(self);   break;
        case Suffix:  IndentParser_Suffix_newline(self);    break;
      }
      break;
    case ' ':
    case '\t':
      switch(self->status)
      {
        case Prefix:  IndentParser_Prefix_space(self);      break;
        case Content: IndentParser_Content_space(self);     break;
        case Suffix:  IndentParser_Suffix_space(self);      break;
      }
      break;
    default:
      switch(self->status)
      {
        case Prefix:  IndentParser_Prefix_character(self);  break;
        case Content: IndentParser_Content_character(self); break;
        case Suffix:  IndentParser_Suffix_character(self);  break;
      }
      break;
  }
}

inline static IndentParser* IndentParser_new(VALUE self)
{
  IndentParser *parser      = xmalloc(sizeof(IndentParser));
  parser->strip             = RTEST(indent_parser_strip(self));
  parser->track_empty_lines = RTEST(indent_parser_track_empty_lines(self));
  parser->root              = rb_ary_new();
  parser->stack_limit       = parser->empty_lines = parser->stack_size = parser->last_prefix = 0;
  parser->stack             = NULL;
  parser->status            = Prefix;
  IndentParser_newline(parser);
  rb_ary_push(parser->root, sMulti);
  return parser;
}

static VALUE indent_parser_compile(VALUE self, VALUE source)
{
  char *start, *end;
  VALUE str             = rb_str_plus(source, rb_str_new("\n", 1));
  IndentParser *parser  = IndentParser_new(self);
  start                 = RSTRING_PTR(str);
  end                   = start + RSTRING_LEN(str);
  parser->content_start = start;
  parser->current       = start;

  IndentParser_add_level(parser, parser->root);
  while(parser->current <= end) {
    IndentParser_process(parser, *parser->current);
    parser->current++;
  }

  VALUE result = parser->root;
  xfree(parser->stack);
  xfree(parser);
  return result;
}

void Init_indent_parser()
{
  // Useful Ruby objects
  sOptions            = rb_intern("@options");
  sStrip              = ID2SYM(rb_intern("strip"));
  sMulti              = ID2SYM(rb_intern("multi"));
  sIndent             = ID2SYM(rb_intern("indent"));
  sTrackEmptyLines    = ID2SYM(rb_intern("track_empty_lines"));

  // Class definition
  VALUE mSlaml        = rb_define_module("Slaml");
  VALUE cIndentParser = rb_define_class_under(mSlaml, "IndentParser", rb_cObject);

  // Ruby API
  rb_define_method(cIndentParser, "initialize",         indent_parser_initialize,        -1);
  rb_define_method(cIndentParser, "compile",            indent_parser_compile,            1);
  rb_define_method(cIndentParser, "strip?",             indent_parser_strip,              0);
  rb_define_method(cIndentParser, "track_empty_lines?", indent_parser_track_empty_lines,  0);
  rb_define_attr(cIndentParser, "options", 1, 1);
}