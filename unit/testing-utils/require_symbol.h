/*******************************************************************\

Module: Unit test utilities

Author: Diffblue Limited.

\*******************************************************************/

#ifndef CPROVER_TESTING_UTILS_REQUIRE_SYMBOL_H
#define CPROVER_TESTING_UTILS_REQUIRE_SYMBOL_H

#include <util/irep.h>

class symbol_tablet;
class symbolt;

/// \file
/// Helper functions for getting symbols from the symbol table during unit tests

// NOLINTNEXTLINE(readability/namespace)
namespace require_symbol
{
#if defined(__GNUC__) && __GNUC__ >= 14
[[gnu::no_dangling]]
#endif
const symbolt &
require_symbol_exists(
  const symbol_tablet &symbol_table,
  const irep_idt &symbol_identifier);
}

#endif // CPROVER_TESTING_UTILS_REQUIRE_SYMBOL_H
