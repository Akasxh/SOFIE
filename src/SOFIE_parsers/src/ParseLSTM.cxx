#include "SOFIE/RModelParser_ONNX.hxx"
#include "SOFIE/ROperator_LSTM.hxx"
#include "onnx_proto3.pb.h"


namespace SOFIE {

ParserFuncSignature ParseLSTM = [](RModelParser_ONNX &parser, const onnx::NodeProto &nodeproto) {
   ETensorType input_type;

   auto input_name = nodeproto.input(0);
   if (parser.IsRegisteredTensorType(input_name)) {
      input_type = parser.GetTensorType(input_name);
   } else {
      throw std::runtime_error("TMVA::SOFIE ONNX Parser LSTM op has input tensor " + input_name +
                               " but its type is not yet registered");
   }

   std::unique_ptr<ROperator> op;

   std::vector<float> attr_activation_alpha;
   std::vector<float> attr_activation_beta;
   std::vector<std::string> attr_activations;
   float attr_clip = 0.;
   std::string attr_direction = "forward";
   size_t attr_hidden_size = 0;
   size_t attr_input_forget = 0;
   size_t attr_layout = 0;

   for (int_t i = 0; i < nodeproto.attribute_size(); i++) {
      std::string attribute_name = nodeproto.attribute(i).name();
      if (attribute_name == "activation_alpha") {
         attr_activation_alpha = {nodeproto.attribute(i).floats().begin(), nodeproto.attribute(i).floats().end()};
      } else if (attribute_name == "activation_beta") {
         attr_activation_beta = {nodeproto.attribute(i).floats().begin(), nodeproto.attribute(i).floats().end()};
      } else if (attribute_name == "activations") {
         attr_activations = {nodeproto.attribute(i).strings().begin(), nodeproto.attribute(i).strings().end()};
      } else if (attribute_name == "clip") {
         attr_clip = nodeproto.attribute(i).f();
      } else if (attribute_name == "direction") {
         attr_direction = nodeproto.attribute(i).s();
      } else if (attribute_name == "hidden_size") {
         attr_hidden_size = nodeproto.attribute(i).i();
      } else if (attribute_name == "input_forget") {
         attr_input_forget = nodeproto.attribute(i).i();
      } else if (attribute_name == "layout") {
         attr_layout = nodeproto.attribute(i).i();
      } else {
         std::cout << "TMVA SOFIE Warning - Model Loading - Attribute " << attribute_name << " in OperatorNode "
                   << nodeproto.name() << " is not defined in ONNX IR and not applied!\n";
      }
   }

   // Optional inputs and outputs
   std::string name_b;
   std::string name_sequence_lens;
   std::string name_initial_h;
   std::string name_initial_c;
   std::string name_p;
   std::string name_y;
   std::string name_y_h;
   std::string name_y_c;
   if (nodeproto.input_size() > 3) {
      name_b = nodeproto.input(3);
   }
   if (nodeproto.input_size() > 4) {
      name_sequence_lens = nodeproto.input(4);
   }
   if (nodeproto.input_size() > 5) {
      name_initial_h = nodeproto.input(5);
   }
   if (nodeproto.input_size() > 6) {
      name_initial_c = nodeproto.input(6);
   }
   if (nodeproto.input_size() > 7) {
      name_p = nodeproto.input(7);
   }
   if (nodeproto.output_size() > 0) {
      name_y = nodeproto.output(0);
   }
   if (nodeproto.output_size() > 1) {
      name_y_h = nodeproto.output(1);
   }
   if (nodeproto.output_size() > 2) {
      name_y_c = nodeproto.output(2);
   }

   switch (input_type) {
   case ETensorType::FLOAT:
      op.reset(new ROperator_LSTM<float>(
         attr_activation_alpha, attr_activation_beta, attr_activations, attr_clip, attr_direction, attr_hidden_size,
         attr_input_forget, attr_layout, nodeproto.input(0), nodeproto.input(1), nodeproto.input(2), name_b,
         name_sequence_lens, name_initial_h, name_initial_c, name_p, name_y, name_y_h, name_y_c));
      break;
   default:
      throw std::runtime_error("TMVA::SOFIE - Unsupported - Operator LSTM does not yet support input type " +
                               std::to_string(static_cast<int>(input_type)));
   }

   for (auto &name : {name_y, name_y_h, name_y_c}) {
      if (!parser.IsRegisteredTensorType(name)) {
         parser.RegisterTensorType(name, input_type);
      }
   }
   return op;
};

} // namespace SOFIE

