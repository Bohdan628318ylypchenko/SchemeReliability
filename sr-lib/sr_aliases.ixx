export module sr_aliases;

export import sr_state;

import std;

using std::function;
using std::vector;

export namespace sr
{
    using SFunc = function<bool(const StateVector&)>;
}
