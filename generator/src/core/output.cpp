#include "feedback/output.h"

#include "feedback/container.h"
#include "feedback/format.h"
#include "feedback/json.h"

#include <ostream>

using fmt::operator""_a;

namespace feedback::output {
  void print(std::ostream& out, output::header const& header) {
    format::print(out,
                  R"_(// DO NOT EDIT: this file is generated automatically

namespace {{ using dummy = int; }}

#define __STRINGIFY(x) #x
#define STRINGIFY(x)   __STRINGIFY(x)
#define PRAGMA(x)      _Pragma(#x)

#if defined __GNUC__
#define FEEDBACK_RESPONSE_ERROR(id, msg)   PRAGMA(GCC error "feedback " STRINGIFY(id) ": " msg)
#define FEEDBACK_RESPONSE_WARNING(id, msg) PRAGMA(GCC warning "feedback " STRINGIFY(id) ": " msg)
#define FEEDBACK_RESPONSE_MESSAGE(id, msg) PRAGMA(message "feedback " STRINGIFY(id) ": " msg)
#define FEEDBACK_RESPONSE_NONE(id, msg)    /* no feedback response for id */
#elif defined _MSC_VER
#define FEEDBACK_MESSAGE(msg)              PRAGMA(message(__FILE__ "(" STRINGIFY(__LINE__) "): " msg))
#define FEEDBACK_RESPONSE_ERROR(id, msg)   FEEDBACK_MESSAGE("feedback error " STRINGIFY(id) ": " msg)
#define FEEDBACK_RESPONSE_WARNING(id, msg) FEEDBACK_MESSAGE("feedback warning " STRINGIFY(id) ": " msg)
#define FEEDBACK_RESPONSE_MESSAGE(id, msg) FEEDBACK_MESSAGE("feedback message " STRINGIFY(id) ": " msg)
#define FEEDBACK_RESPONSE_NONE(id, msg)    /* no feedback response for id */
#else
#error "Unsupported compiler"
#endif

)_");

    for (auto [id, rule] : header.shared_rules.get())
      format::print(out,
                    R"_(#define FEEDBACK_MATCH_{uppercase_id}(match, highlighting) FEEDBACK_RESPONSE_{response}({id}, "{summary} [{type} from file://{origin}]\n |\n | " match "\n | " highlighting "\n |\n | RATIONALE : {rationale}\n | WORKAROUND: {workaround}\n |")
)_",
                    "origin"_a = format::as_literal{ header.rules_origin }, "id"_a = id,
                    "uppercase_id"_a = format::uppercase{ id },
                    "response"_a     = format::uppercase{ json::to_string(
                    container::value_or_default(header.shared_workflow.get(), rule.type).response) },
                    "type"_a = format::as_literal{ rule.type }, "summary"_a = format::as_literal{ rule.summary },
                    "rationale"_a = format::as_literal{ rule.rationale }, "workaround"_a = format::as_literal{ rule.workaround });
  }
} // namespace feedback::output
