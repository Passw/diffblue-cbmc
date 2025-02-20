/*******************************************************************\

Module: C++ Language Type Checking

Author: Daniel Kroening, kroening@cs.cmu.edu

\*******************************************************************/

/// \file
/// C++ Language Type Checking

#include <util/c_types.h>
#include <util/cprover_prefix.h>
#include <util/mathematical_types.h>
#include <util/simplify_expr.h>
#include <util/source_location.h>

#include <ansi-c/c_qualifiers.h>
#include <ansi-c/merged_type.h>

#include "cpp_convert_type.h"
#include "cpp_typecheck.h"
#include "cpp_typecheck_fargs.h"

void cpp_typecheckt::typecheck_type(typet &type)
{
  PRECONDITION(!type.id().empty());
  PRECONDITION(type.is_not_nil());

  try
  {
    cpp_convert_plain_type(type, get_message_handler());
  }

  catch(const char *err)
  {
    error().source_location=type.source_location();
    error() << err << eom;
    throw 0;
  }

  catch(const std::string &err)
  {
    error().source_location=type.source_location();
    error() << err << eom;
    throw 0;
  }

  if(type.id()==ID_cpp_name)
  {
    c_qualifierst qualifiers(type);

    cpp_namet cpp_name;
    cpp_name.swap(type);

    exprt symbol_expr=resolve(
      cpp_name,
      cpp_typecheck_resolvet::wantt::TYPE,
      cpp_typecheck_fargst());

    if(symbol_expr.id()!=ID_type)
    {
      error().source_location=type.source_location();
      error() << "expected type" << eom;
      throw 0;
    }

    type=symbol_expr.type();
    PRECONDITION(type.is_not_nil());

    if(type.get_bool(ID_C_constant))
      qualifiers.is_constant = true;

    // CPROVER extensions
    irep_idt typedef_identifier = type.get(ID_C_typedef);
    if(typedef_identifier == CPROVER_PREFIX "rational")
    {
      type = rational_typet();
      type.add_source_location() = symbol_expr.source_location();
    }
    else if(typedef_identifier == CPROVER_PREFIX "integer")
    {
      type = integer_typet();
      type.add_source_location() = symbol_expr.source_location();
    }

    qualifiers.write(type);
  }
  else if(type.id()==ID_struct ||
          type.id()==ID_union)
  {
    typecheck_compound_type(to_struct_union_type(type));
  }
  else if(type.id()==ID_pointer)
  {
    c_qualifierst qualifiers(type);

    // the pointer/reference might have a qualifier,
    // but do subtype first
    typecheck_type(to_pointer_type(type).base_type());

    // Check if it is a pointer-to-member
    if(type.find(ID_to_member).is_not_nil())
    {
      // these can point either to data members or member functions
      // of a class

      typet &class_object = static_cast<typet &>(type.add(ID_to_member));

      if(class_object.id()==ID_cpp_name)
      {
        DATA_INVARIANT(
          class_object.get_sub().back().id() == "::", "scope suffix expected");
        class_object.get_sub().pop_back();
      }

      typecheck_type(class_object);

      // there may be parameters if this is a pointer to member function
      if(to_pointer_type(type).base_type().id() == ID_code)
      {
        code_typet::parameterst &parameters =
          to_code_type(to_pointer_type(type).base_type()).parameters();

        if(parameters.empty() || !parameters.front().get_this())
        {
          // Add 'this' to the parameters
          code_typet::parametert a0(pointer_type(class_object));
          a0.set_base_name(ID_this);
          a0.set_this();
          parameters.insert(parameters.begin(), a0);
        }
      }
    }

    if(type.get_bool(ID_C_constant))
      qualifiers.is_constant = true;

    qualifiers.write(type);
  }
  else if(type.id()==ID_array)
  {
    exprt &size_expr=to_array_type(type).size();

    if(size_expr.is_not_nil())
    {
      typecheck_expr(size_expr);
      simplify(size_expr, *this);
    }

    typecheck_type(to_array_type(type).element_type());

    if(to_array_type(type).element_type().get_bool(ID_C_constant))
      type.set(ID_C_constant, true);

    if(to_array_type(type).element_type().get_bool(ID_C_volatile))
      type.set(ID_C_volatile, true);
  }
  else if(type.id()==ID_vector)
  {
    // already done
  }
  else if(type.id() == ID_frontend_vector)
  {
    typecheck_vector_type(type);
  }
  else if(type.id()==ID_code)
  {
    code_typet &code_type=to_code_type(type);
    typecheck_type(code_type.return_type());

    code_typet::parameterst &parameters=code_type.parameters();

    for(auto &param : parameters)
    {
      typecheck_type(param.type());

      // see if there is a default value
      if(param.has_default_value())
      {
        typecheck_expr(param.default_value());
        implicit_typecast(param.default_value(), param.type());
      }
    }
  }
  else if(type.id()==ID_template)
  {
    typecheck_type(to_template_type(type).subtype());
  }
  else if(type.id()==ID_c_enum)
  {
    typecheck_enum_type(type);
  }
  else if(type.id()==ID_c_enum_tag)
  {
  }
  else if(type.id()==ID_c_bit_field)
  {
    typecheck_c_bit_field_type(to_c_bit_field_type(type));
  }
  else if(
    type.id() == ID_unsignedbv || type.id() == ID_signedbv ||
    type.id() == ID_bool || type.id() == ID_c_bool || type.id() == ID_floatbv ||
    type.id() == ID_fixedbv || type.id() == ID_empty)
  {
  }
  else if(type.id() == ID_struct_tag)
  {
  }
  else if(type.id() == ID_union_tag)
  {
  }
  else if(type.id()==ID_constructor ||
          type.id()==ID_destructor)
  {
  }
  else if(type.id()=="cpp-cast-operator")
  {
  }
  else if(type.id()=="cpp-template-type")
  {
  }
  else if(type.id()==ID_typeof)
  {
    exprt e=static_cast<const exprt &>(type.find(ID_expr_arg));

    if(e.is_nil())
    {
      typet tmp_type=
        static_cast<const typet &>(type.find(ID_type_arg));

      if(tmp_type.id()==ID_cpp_name)
      {
        // this may be ambiguous -- it can be either a type or
        // an expression

        cpp_typecheck_fargst fargs;

        exprt symbol_expr=resolve(
          to_cpp_name(static_cast<const irept &>(tmp_type)),
          cpp_typecheck_resolvet::wantt::BOTH,
          fargs);

        type=symbol_expr.type();
      }
      else
      {
        typecheck_type(tmp_type);
        type=tmp_type;
      }
    }
    else
    {
      typecheck_expr(e);
      type=e.type();
    }
  }
  else if(type.id()==ID_decltype)
  {
    exprt e=static_cast<const exprt &>(type.find(ID_expr_arg));
    typecheck_expr(e);

    if(e.type().id() == ID_c_bit_field)
      type = to_c_bit_field_type(e.type()).underlying_type();
    else
      type = e.type();
  }
  else if(type.id()==ID_unassigned)
  {
    // ignore, for template parameter guessing
  }
  else if(type.id()==ID_template_class_instance)
  {
    // ok (internally generated)
  }
  else if(type.id()==ID_block_pointer)
  {
    // This is an Apple extension for lambda-like constructs.
    // http://thirdcog.eu/pwcblocks/
    // we just treat them as references to functions
    type.id(ID_frontend_pointer);
    typecheck_type(type);
  }
  else if(type.id()==ID_nullptr)
  {
  }
  else if(type.id()==ID_already_typechecked)
  {
    c_typecheck_baset::typecheck_type(type);
  }
  else if(type.id() == ID_gcc_attribute_mode)
  {
    PRECONDITION(type.has_subtype());
    merged_typet as_parsed;
    as_parsed.move_to_subtypes(to_type_with_subtype(type).subtype());
    type.get_sub().clear();
    as_parsed.move_to_subtypes(type);
    type.swap(as_parsed);

    c_typecheck_baset::typecheck_type(type);
  }
  else if(type.id() == ID_complex)
  {
    // already done
  }
  else
  {
    error().source_location=type.source_location();
    error() << "unexpected cpp type: " << type.pretty() << eom;
    throw 0;
  }

  CHECK_RETURN(type.is_not_nil());
}
