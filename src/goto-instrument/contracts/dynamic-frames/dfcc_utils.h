/*******************************************************************\

Module: Dynamic frame condition checking for function contracts

Author: Remi Delmas, delmasrd@amazon.com
Date: August 2022

\*******************************************************************/

/// \file
/// Dynamic frame condition checking utility functions

#ifndef CPROVER_GOTO_INSTRUMENT_CONTRACTS_DYNAMIC_FRAMES_DFCC_UTILS_H
#define CPROVER_GOTO_INSTRUMENT_CONTRACTS_DYNAMIC_FRAMES_DFCC_UTILS_H

#include <util/message.h>
#include <util/namespace.h>
#include <util/std_expr.h>

#include <set>

class goto_modelt;
class goto_programt;
class message_handlert;
class symbolt;

struct dfcc_utilst
{
  /// Returns true iff the given symbol exists and satisfies requirements.
  static bool
  function_symbol_exists(const goto_modelt &, const irep_idt &function_id);
  static bool function_symbol_with_body_exists(
    const goto_modelt &,
    const irep_idt &function_id);

  /// Returns the `symbolt` for `function_id`.
  static symbolt &
  get_function_symbol(symbol_table_baset &, const irep_idt &function_id);

  /// Adds a new symbol named `function_id::base_name` of type `type`
  /// with given attributes in the symbol table, and returns a symbol expression
  /// for the created symbol.
  /// If a symbol of the same name already exists the name will be decorated
  /// with a unique suffix.
  static symbol_exprt create_symbol(
    symbol_table_baset &,
    const typet &type,
    const irep_idt &function_id,
    const std::string &base_name,
    const source_locationt &source_location);

  /// Adds a new static symbol named `prefix::base_name` of type `type` with
  /// value `initial_value` in the symbol table, returns the created symbol.
  /// If a symbol of the same name already exists the name will be decorated
  /// with a unique suffix.
  /// By default the symbol is be protected against nondet initialisation
  /// by tagging the its value field with a ID_C_no_nondet_initialization
  /// attribute as understood by \ref nondet_static.
  /// This is because the static instrumentation variables are meant to
  /// keep their initial values for the instrumentation to work as intended.
  /// To allow nondet-initialisation of the symbol,
  /// just set  \p no_nondet_initialization to `false` when calling the method.
  static const symbolt &create_static_symbol(
    symbol_table_baset &,
    const typet &type,
    const std::string &prefix,
    const std::string &base_name,
    const source_locationt &source_location,
    const irep_idt &mode,
    const irep_idt &module,
    const exprt &initial_value,
    const bool no_nondet_initialization = true);

  /// Creates a new parameter symbol for the given function_id
#if defined(__GNUC__) && __GNUC__ >= 14
  [[gnu::no_dangling]]
#endif
  static const symbolt &
  create_new_parameter_symbol(
    symbol_table_baset &,
    const irep_idt &function_id,
    const std::string &base_name,
    const typet &type);

  /// \brief Adds the given symbol as parameter to the function symbol's
  /// code_type. Also adds the corresponding parameter to its goto_function if
  /// it exists in the function map of the goto model.
  static void add_parameter(
    goto_modelt &,
    const symbolt &symbol,
    const irep_idt &function_id);

  /// \brief Adds a parameter with given `base_name` and `type` to the given
  /// `function_id`. Both the symbol and the goto_function are updated.
  static const symbolt &add_parameter(
    goto_modelt &,
    const irep_idt &function_id,
    const std::string &base_name,
    const typet &type);

  /// \brief Creates a new function symbol and goto_function
  /// entry in the `goto_functions_map` by cloning the given `function_id`.
  ///
  /// The cloned function symbol has `new_function_id` as name
  /// The cloned parameters symbols have `new_function_id::name` as name
  /// Returns the new function symbol
#if defined(__GNUC__) && __GNUC__ >= 14
  [[gnu::no_dangling]]
#endif
  static const symbolt &
  clone_and_rename_function(
    goto_modelt &goto_model,
    const irep_idt &function_id,
    const irep_idt &new_function_id,
    std::optional<typet> new_return_type);

  /// Given a function to wrap `foo` and a new name `wrapped_foo`
  ///
  /// ```
  /// ret_t foo(x_t foo::x, y_t foo::y) { foo_body; }
  /// ```
  ///
  /// This method creates a new entry in the symbol_table for
  /// `wrapped_foo` and moves the body of the original function, `foo_body`,
  /// under `wrapped_foo` in `function_map`.
  ///
  /// The parameter symbols of `wrapped_foo` remain the same as in `foo`:
  /// ```
  /// ret_t wrapped_foo(x_t foo::x, y_t foo::y) { foo_body; }
  /// ```
  ///
  /// The parameters of the original `foo` get renamed to
  /// make sure they are distinct from that of `wrapped_foo`, and a new
  /// empty body is generated for `foo`:
  ///
  /// ```
  /// ret_t foo(x_t foo::x_wrapped, y_t foo::y_wrapped) { };
  /// ```
  ///
  /// Any other goto_function calling `foo` still calls `foo` which has become
  /// a wrapper for `wrapped_foo`.
  ///
  /// By generating a new body for `foo`, one can implement contract
  /// checking logic, contract replacement logic, etc.
  static void wrap_function(
    goto_modelt &goto_model,
    const irep_idt &function_id,
    const irep_idt &wrapped_function_id);

  /// \brief Returns the expression `expr == NULL`.
  static const exprt make_null_check_expr(const exprt &ptr);

  /// \brief Returns the expression `sizeof(expr)`.
  static exprt make_sizeof_expr(const exprt &expr, const namespacet &);

  /// \brief Inlines the given function, aborts on recursive calls during
  /// inlining.
  static void inline_function(
    goto_modelt &goto_model,
    const irep_idt &function_id,
    message_handlert &message_handler);

  /// \brief Inlines the given function, and returns function symbols that
  /// caused warnings.
  static void inline_function(
    goto_modelt &goto_model,
    const irep_idt &function_id,
    std::set<irep_idt> &no_body,
    std::set<irep_idt> &recursive_call,
    std::set<irep_idt> &missing_function,
    std::set<irep_idt> &not_enough_arguments,
    message_handlert &message_handler);

  /// \brief Inlines the given program, and returns function symbols that
  /// caused warnings.
  static void inline_program(
    goto_modelt &goto_model,
    goto_programt &goto_program,
    std::set<irep_idt> &no_body,
    std::set<irep_idt> &recursive_call,
    std::set<irep_idt> &missing_function,
    std::set<irep_idt> &not_enough_arguments,
    message_handlert &message_handler);
};

#endif
