#include <string>

#include <wrutil/Format.h>
#include <wrutil/numeric_cast.h>
#include <wrutil/tagged_ptr.h>


namespace wr {
namespace fmt {


void
TaggedPtrHandlerBase::set(
        Arg       &arg,
        void      *ptr,
        uintptr_t  tag
) // static
{
        arg.type = Arg::OTHER_T;
        arg.other = ptr;
        arg.s.length = tag;
        arg.fmt_fn = &format;
}

//--------------------------------------

bool
TaggedPtrHandlerBase::format(
        const Params &parms
) // static
{
        Arg  arg2;
        char buf[48];

        switch (parms.conv) {
        case 's':
                arg2.type = Arg::STR_T;
                arg2.s.data = buf;
                if (parms.arg->other) {
                        arg2.s.length = numeric_cast<size_t>(
                                wr::print(buf, sizeof(buf), "{0x%x, %u}",
                                          parms.arg->other,
                                          parms.arg->s.length));
                } else {
                        arg2.s.length = numeric_cast<size_t>(
                                wr::print(buf, sizeof(buf), "{nullptr, %u}",
                                          parms.arg->s.length));
                }
                break;
        default:
                // format pointer using default semantics
                arg2 = *parms.arg;
                arg2.fmt_fn = nullptr;
                break;
        }

        return parms.target.format(parms, &arg2);
}


} // namespace fmt
} // namespace wr
