/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*!
 * \file tvm/relay/op_attr_types.h
 * \brief The Expr and related elements in DataFlow construction.
 */
#ifndef TVM_RELAY_OP_ATTR_TYPES_H_
#define TVM_RELAY_OP_ATTR_TYPES_H_

#include <tvm/tensor.h>
#include <tvm/schedule.h>
#include <tvm/build_module.h>
#include <tvm/relay/type.h>
#include <tvm/relay/expr.h>
#include <string>

namespace tvm {
namespace relay {

/*! \brief operator pattern used in graph fusion */
enum OpPatternKind {
  // Elementwise operation
  kElemWise = 0,
  // Broadcasting operator, can always map output axis to the input in order.
  // for example :code:`out[i, ax1, j, ax2] = input[i, j]`.
  // Note that the axis need to be in order so transpose is not a bcast operator.
  kBroadcast = 1,
  // Injective operator, can always injectively map output axis to a single input axis.
  // All injective operator can still be safely fused to injective and reduction.
  kInjective = 2,
  // Communicative reduction operator.
  kCommReduce = 3,
  // Complex operation, can still fuse elemwise operations into its output.
  // but cannot chain another complex op
  kOutEWiseFusable = 4,
  // The pattern for tuple nodes. Can fuse into subsequent injective ops,
  // but treated specially
  kTuple = 7,
  // Opaque operation, cannot fuse anything.
  kOpaque = 8
};

/*! \brief the operator pattern */
using TOpPattern = int;

/*!
 * \brief Whether operator is stateful or contain internal state.
 *
 * All the primitive ops we registered so far are pure.
 * This attribute is left for potential future compatible reasons.
 * We can always work around the stateful ops by adding an additional
 * handle argument and return it.
 */
using TOpIsStateful = bool;

/*!
 * \brief Mark the operator as non-computational.
 */
using TNonComputational = bool;

/*!
 * \brief Mark the operator whether output shape is data dependant.
 */
using TShapeDataDependant = bool;

/*!
 * \brief Computation description interface.
 *
 * \note This function have a special convention
 *  for functions with tuple input/output.
 *
 *  So far we restrict tuple support to the following case:
 *  - Function which takes a single tuple as input.
 *  - Function which outputs a single tuple.
 *
 *  In both cases, the tuple is flattened as array.
 *
 * \param attrs The attribute of the primitive
 * \param inputs The input tensors.
 * \param out_type The output type information
 &                 these are always placeholders.
 * \return The output compute description of the operator.
 */
using FTVMCompute = runtime::TypedPackedFunc<
  Array<Tensor>(const Attrs& attrs,
                const Array<Tensor>& inputs,
                const Type& out_type)>;

/*!
 * \brief Build the computation schedule for
 *  op whose root is at current op.
 *
 * \param attrs The attribute of the node.
 * \param outs The output tensors.
 * \param target The build target.
 * \return schedule The computation schedule.
 */
using FTVMSchedule = runtime::TypedPackedFunc<
    Schedule(const Attrs& attrs,
             const Array<Tensor>& outs,
             const Target& target)>;

/*!
 * \brief Generate the strategy of operators. This function is a generic
 * function and can be re-defined for different targets.
 *
 * The function signature of generic function is:
 *   OpStrategy(const Attrs& attrs, const Array<Tensor>& inputs,
 *              const Type& out_type, const Target& target)
 */
using FTVMStrategy = GenericFunc;

/*!
 * \brief Alternate the layout of operators or replace the
 *  operator with other expressions. This function will be invoked
 *  in AlterOpLayout pass.
 * \param attrs The attribute of the original node.
 * \param inputs The input symbols of the original node.
 * \param tinfos An array of placeholders, use for getting the inferred shape
 *               and dtype of the inputs.
 * \return new_expr The modified expression.
 */
using FTVMAlterOpLayout = runtime::TypedPackedFunc<
  Expr(const Attrs& attrs,
       const Array<Expr>& args,
       const Array<Tensor>& tinfos)>;

/*!
 * \brief Convert the layout of operators or replace the
 *  operator with other expressions. This function will be invoked
 *  in ConvertLayout pass.
 * \param attrs The attribute of the original node.
 * \param inputs The input symbols of the original node.
 * \param tinfos An array of placeholders, use for getting the inferred shape
 *               and dtype of the inputs.
 * \param desired_layout The desired layout.
 * \return new_expr The modified expression.
 */
using FTVMConvertOpLayout = runtime::TypedPackedFunc<
  Expr(const Attrs& attrs,
       const Array<Expr>& args,
       const Array<Tensor>& tinfos,
       const std::string& desired_layout)>;
/*!
 * \brief Legalizes an expression with another expression. This function will be
 *  invoked in Legalize pass. It is a target-dependent pass.
 * \param attrs The attribute of the original node.
 * \param inputs The input symbols of the original node.
 * \param tinfos An array of placeholders, use for getting the inferred shape
 *               and dtype of the inputs.
 * \return new_expr The modified expression.
 */
using FTVMLegalize = runtime::TypedPackedFunc<
  Expr(const Attrs& attrs,
       const Array<Expr>& args,
       const Array<tvm::relay::Type>& arg_types)>;

/*!
 * \brief Forward rewriting rule for a specific op.
 *
 * \param ref_call The reference old call type to be rewritten.
 *                 We can make use of the op and type information.
 * \param new_args The new arguments (some of them could be TempExpr).
 * \param ctx  Optional context information about ref_call.
 * \return The rewriten result call, can also return nullptr,
 *         which indicate the rewriter should use the default fallback
 *         rule that realizes all its input and compose the call.
 *
 * \note When we register the function, we can register
 *       a different signature with ctx to be a specific node type.
 */
using FForwardRewrite = runtime::TypedPackedFunc<
  Expr(const Call& ref_call,
       const Array<Expr>& new_args,
       const ObjectRef& ctx)>;

/*!
 * \brief Gradient for a specific op.
 *
 * \param orig_call the original Expr.
 * \param output_grad the gradient of the Expr.
 * \return the gradient for each parameters.
 */
using FPrimalGradient = runtime::TypedPackedFunc<tvm::Array<Expr>(const Expr& orig_call,
                                                                  const Expr& output_grad)>;

/*!
 * \brief The codegeneration strategy for dynamic dimensions.
 */
enum AnyCodegenStrategy {
  /*! \brief The default strategy of using completely variable dimensions. */
  kVariableDimensions
};

/*! \brief A runtime representation of shape. */
using Shape = Array<IndexExpr>;

using FShapeFunc = runtime::TypedPackedFunc<
  Array<Tensor>(const Attrs& attrs,
                const Array<Tensor>& inputs,
                const Array<IndexExpr>& out_ndims)>;

/*!
 * \brief Operator implementation in TVM.
 */
class OpImplementNode : public Object {
 public:
  /*! \brief Compute function */
  FTVMCompute fcompute;
  /*! \brief Schedule function */
  FTVMSchedule fschedule;
  /*! \brief Priority level */
  Integer plevel;

  void VisitAttrs(tvm::AttrVisitor* v) {
    v->Visit("plevel", &plevel);
  }

  static constexpr const char* _type_key = "relay.OpImplement";
  TVM_DECLARE_FINAL_OBJECT_INFO(OpImplementNode, Object);
};

/*!
 * \brief Operator implementation class.
 */
class OpImplement : public ObjectRef {
 public:
  /*! \brief default constructor  */
  OpImplement() {}
  /*! \brief constructor from node pointer */
  explicit OpImplement(ObjectPtr<Object> n) : ObjectRef(n) {}
  /*!
   * \brief access the internal node container
   * \return the pointer to the internal node container
   */
  inline const OpImplementNode* operator->() const;
  /*!
   * \brief Invoke the operator compute function.
   * \param attrs The attribute of the primitive
   * \param inputs The input tensors.
   * \param out_type The output type information.
   * \return The output compute description of the operator.
   */
  Array<Tensor> Compute(const Attrs& attrs,
                        const Array<Tensor>& inputs,
                        const Type& out_type);
  /*!
   * \brief Build the computation schedule.
   * \param attrs The attribute of the node.
   * \param outs The output tensors.
   * \param target The build target.
   * \return The computation schedule.
   */
  Schedule Schedule(const Attrs& attrs,
                    const Array<Tensor>& outs,
                    const Target& target);
};

/*!
 * \brief Specialized implementations for operators under certain conditions.
 */
class OpSpecializationNode : public Object {
 public:
  /*! \brief List of implementations. */
  Array<OpImplement> implements;
  /*! \brief Condition to enable the specialization.
   *    Could be undefined to represent generic case. */
  SpecializedCondition condition;

  void VisitAttrs(tvm::AttrVisitor* v) {
    v->Visit("condition", &condition);
    v->Visit("implements", &implements);
  }

  static constexpr const char* _type_key = "relay.OpSpecialization";
  TVM_DECLARE_FINAL_OBJECT_INFO(OpSpecializationNode, ExprNode);
};

/*!
 * \brief Operator specialization class.
 */
class OpSpecialization : public ObjectRef {
 public:
  OpSpecialization() {}
  explicit OpSpecialization(ObjectPtr<Object> n) : ObjectRef(n) {}
  /*!
   * \brief access the internal node container
   * \return the pointer to the internal node container
   */
  inline const OpSpecializationNode* operator->() const;
  /*!
   * \brief access the internal node container
   * \return the pointer to the internal node container
   */
  inline OpSpecializationNode* operator->();
  /*!
   * \brief Add an implementation.
   * \param compute Compute function
   * \param schedule Schedule function
   * \param plevel Priority level of this implemntation.
   */
  void AddImplement(FTVMCompute fcompute, FTVMSchedule fschedule,
                    int plevel);
};

/*!
 * \brief Operator strategy to choose implementation.
 */
class OpStrategyNode : public Object {
 public:
  /*! \brief List of operator specializations. */
  Array<OpSpecialization> specializations;

  void VisitAttrs(tvm::AttrVisitor* v) {
    v->Visit("specializations", &specializations);
  }

  static constexpr const char* _type_key = "relay.OpStrategy";
  TVM_DECLARE_FINAL_OBJECT_INFO(OpStrategyNode, ExprNode);
};

/*!
 * \brief Operator strategy class.
 */
class OpStrategy : public ObjectRef {
 public:
  /*! \brief default constructor  */
  OpStrategy() {}
  /*! \brief constructor from node pointer */
  explicit OpStrategy(ObjectPtr<Object> n) : ObjectRef(n) {}
  /*!
   * \brief access the internal node container
   * \return the pointer to the internal node container
   */
  inline const OpStrategyNode* operator->() const;
  /*!
   * \brief access the internal node container
   * \return the pointer to the internal node container
   */
  inline OpStrategyNode* operator->();
  /*!
   * \brief Add an implementation.
   * \param compute Compute function
   * \param schedule Schedule function
   * \param plevel Priority level of this implementation.
   */
  void AddImplement(FTVMCompute fcompute, FTVMSchedule fschedule, int plevel);
};

// implementations
inline const OpImplementNode* OpImplement::operator->() const {
  return static_cast<const OpImplementNode*>(get());
}

inline const OpSpecializationNode* OpSpecialization::operator->() const {
  return static_cast<const OpSpecializationNode*>(get());
}

inline OpSpecializationNode* OpSpecialization::operator->() {
  return static_cast<OpSpecializationNode*>(get_mutable());
}

inline const OpStrategyNode* OpStrategy::operator->() const {
  return static_cast<const OpStrategyNode*>(get());
}

inline OpStrategyNode* OpStrategy::operator->() {
  return static_cast<OpStrategyNode*>(get_mutable());
}

}  // namespace relay
}  // namespace tvm
#endif  // TVM_RELAY_OP_ATTR_TYPES_H_
