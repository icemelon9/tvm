#include <tvm/relay/op_attr_types.h>

namespace tvm {
namespace relay {

TVM_REGISTER_NODE_TYPE(OpImplementNode);
TVM_REGISTER_NODE_TYPE(OpSpecializationNode);
TVM_REGISTER_NODE_TYPE(OpStrategyNode);

Array<Tensor> OpImplement::Compute(const Attrs& attrs,
                                   const Array<Tensor>& inputs,
                                   const Type& out_type) {
  return (*this)->fcompute(attrs, inputs, out_type);
}

Schedule OpImplement::Schedule(const Attrs& attrs,
                               const Array<Tensor> &outs,
                               const Target& target) {
  return (*this)->fschedule(attrs, outs, target);
}

void OpSpecialization::AddImplement(tvm::relay::FTVMCompute fcompute,
                                    tvm::relay::FTVMSchedule fschedule,
                                    int plevel) {
  auto n = make_object<OpImplementNode>();
  n->fcompute = fcompute;
  n->fschedule = fschedule;
  n->plevel = IntImm::make(DataType::Int(32), plevel);
  (*this)->implements.push_back(OpImplement(n));
}

void OpStrategy::AddImplement(FTVMCompute fcompute,
                              FTVMSchedule fschedule,
                              int plevel) {
  auto curr_cond = SpecializedCondition::Current();
  auto specializations = (*this)->specializations;
  OpSpecialization op_spec;
  for (auto e : specializations) {
    if (e->condition == curr_cond) {
      op_spec = e;
      break;
    }
  }
  if (op_spec.defined()) {
    op_spec.AddImplement(fcompute, fschedule, plevel);
  } else {
    ObjectPtr<OpSpecializationNode> n = make_object<OpSpecializationNode>();
    n->condition = curr_cond;
    op_spec = OpSpecialization(n);
    op_spec.AddImplement(fcompute, fschedule, plevel);
    (*this)->specializations.push_back(op_spec);
  }
}

TVM_REGISTER_GLOBAL("relay.op._OpImplementCompute")
.set_body([](TVMArgs args, TVMRetValue* rv) {
    OpImplement imp = args[0];
    Attrs attrs = args[1];
    Array<Tensor> inputs = args[2];
    Type out_type = args[3];
    *rv = imp.Compute(attrs, inputs, out_type);
});

TVM_REGISTER_GLOBAL("relay.op._OpImplementSchedule")
.set_body([](TVMArgs args, TVMRetValue* rv) {
    OpImplement imp = args[0];
    Attrs attrs = args[1];
    Array<Tensor> outs = args[2];
    Target target = args[3];
    *rv = imp.Schedule(attrs, outs, target);
});

TVM_REGISTER_GLOBAL("relay.op._make.OpStrategy")
.set_body([](TVMArgs args, TVMRetValue* rv) {
    ObjectPtr<OpStrategyNode> n = make_object<OpStrategyNode>();
    *rv = OpStrategy(n);
});

TVM_REGISTER_GLOBAL("relay.op._OpStrategyAddImplement")
.set_body([](TVMArgs args, TVMRetValue* rv) {
    OpStrategy strategy = args[0];
    FTVMCompute compute = args[1];
    FTVMSchedule schedule = args[2];
    int plevel = args[3];
    strategy.AddImplement(compute, schedule, plevel);
});


} // namespace relay
} // namespace tvm
