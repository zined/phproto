#ifndef PHPROTO_H
#define PHPROTO_H

typedef struct _phproto_message {
    unsigned id;
    char* name;
} phproto_message;

#ifndef ___I_SHALL_BE_GENERATED___
static const phproto_message phproto_messages[] =
{
    { 1, "SomeMessageRequest" },
    { 2, "SomeMessageResponse" },
};
#endif // ___I_SHALL_BE_GENERATED___

#endif
