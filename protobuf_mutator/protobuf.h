#ifndef PORT_PROTOBUF_H_
#define PORT_PROTOBUF_H_

#include <string>

#include "google/protobuf/any.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/message_differencer.h"
#include "google/protobuf/wire_format.h"

namespace protobuf_mutator {

    namespace protobuf = google::protobuf;

    // String type used by google::protobuf.
    using String = std::string;

}  // namespace protobuf_mutator

#endif  // PORT_PROTOBUF_H_
