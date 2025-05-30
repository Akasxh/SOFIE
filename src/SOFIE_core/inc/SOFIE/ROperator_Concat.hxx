#ifndef SOFIE_ROPERATOR_Concat
 #define SOFIE_ROPERATOR_Concat


 #include "SOFIE/SOFIE_common.hxx"
 #include "SOFIE/ROperator.hxx"
 #include "SOFIE/RModel.hxx"

 #include <sstream>
 #include <algorithm>
 #include <iterator>
 #include <iomanip>
 #include <limits>

 namespace SOFIE{

     class ROperator_Concat final : public ROperator
     {
     private:
         int fAxis=0;
         int fnewAxis=0;
         std::vector<std::string> fInputs;
         std::string fOutput;
         std::vector<Dim>fOutputShape;
         std::vector<std::vector<Dim>> fInputShapes;

     public:
         ROperator_Concat(){}
         ROperator_Concat(std::vector<std::string> inputs, int axis, int newAxis, std::string output):
         fAxis(axis), fnewAxis(newAxis), fOutput(UTILITY::Clean_name(output)) {
            fInputs.reserve(inputs.size());
            for (auto & name : inputs)
               fInputs.push_back(UTILITY::Clean_name(name));

         fInputTensorNames.resize(fInputs.size());
         std::transform(fInputs.begin(), fInputs.end(), fInputTensorNames.begin(),
                   [](const std::string& s) -> std::string_view { return s; });
         fOutputTensorNames = { fOutput };
         }

         std::vector<ETensorType> TypeInference(std::vector<ETensorType> input) override {
             return input;
         }

         // get shape of output given inputs. It is going to be called after initialized
         std::vector<std::vector<size_t>> ShapeInference(std::vector<std::vector<size_t>> inputs) override {
             std::vector<std::vector<size_t>> ret(1);
            // treat negative axis case
            if (fAxis<0) {
               fAxis = inputs[0].size()+fAxis;
            }
            if (fAxis < 0 || fAxis >= (int) inputs[0].size())
               throw std::runtime_error("TMVA SOFIE Concat Op - invalid axis value ");

            int concat_dim=0;
            if(fnewAxis == 0){
               for (size_t i = 0; i < inputs.size(); i++) {
                  if (i > 0 && inputs[i].size() != inputs[i - 1].size())
                     throw std::runtime_error("TMVA SOFIE Concat Op - input tensors have different shapes " +
                                              ConvertShapeToString(inputs[i]) + " and " + ConvertShapeToString(inputs[i - 1]));
                  for (size_t iaxis = 0; iaxis < inputs[i].size(); iaxis++) {
                     if ((int)iaxis == fAxis)
                        concat_dim += inputs[i][iaxis];
                     else if (i > 0 && inputs[i][iaxis] != inputs[i - 1][iaxis])
                        throw std::runtime_error("TMVA SOFIE Concat Op - input tensors have wrong shapes " +
                                                 ConvertShapeToString(inputs[i]) + " and " +
                                                 ConvertShapeToString(inputs[i - 1]));
                  }
               }

               // output shape
               ret[0] = inputs[0];
               ret[0][fAxis] = concat_dim;
            }
            std::vector<int> stack;
            if(fnewAxis == 1){
               for(size_t i = 0; i < inputs.size(); i++) {
                  if (i > 0 && inputs[i].size() != inputs[i-1].size() )
                  throw std::runtime_error("TMVA SOFIE Concat Op - input tensors have different shapes " + fInputs[i] + " : " +
                     ConvertShapeToString(inputs[i]) + " and " + fInputs[i-1] + " : " + ConvertShapeToString(inputs[i-1]));
                  for (size_t iaxis = 0; iaxis < inputs[i].size(); iaxis++) {
                     if ((int) iaxis == fAxis)
                        stack.push_back(inputs[i][iaxis]);
                     else
                     if (i> 0 && inputs[i][iaxis] != inputs[i-1][iaxis])
                        throw std::runtime_error("TMVA SOFIE Concat Op - input tensors have wrong shapes " +
                        ConvertShapeToString(inputs[i]) + " and " + ConvertShapeToString(inputs[i-1]));
                  }

               }
               for(auto it:stack)
               ret[0].push_back(it);
            }

            return ret;
         }

         // get shape of output given inputs. It is going to be called after initialized
         std::vector<std::vector<Dim>> ShapeInference(const std::vector<std::vector<Dim>> & inputs) {
            std::vector<std::vector<Dim>> ret(1);
            // treat negative axis case
            if (fAxis<0) {
               fAxis = inputs[0].size()+fAxis;
            }
            if (fAxis < 0 || fAxis >= (int) inputs[0].size())
               throw std::runtime_error("TMVA SOFIE Concat Op - invalid axis value ");

            int concat_dim=0;
            if(fnewAxis == 0){
               for (size_t i = 0; i < inputs.size(); i++) {
                  if (i > 0 && inputs[i].size() != inputs[i - 1].size())
                     throw std::runtime_error("TMVA SOFIE Concat Op - input tensors have different shapes " + fInputs[i] + " : " +
                                              ConvertDynamicShapeToString(inputs[i]) + " and " + fInputs[i-1] + " : " + ConvertDynamicShapeToString(inputs[i - 1]));
                  for (size_t iaxis = 0; iaxis < inputs[i].size(); iaxis++) {
                     if ((int)iaxis == fAxis) {
                        // support only non-params shape for the concatenation axis
                        if (inputs[i][iaxis].isParam)
                           throw std::runtime_error("TMVA SOFIE Concat Op - not supporting input param dimensions for concatenation axis. Input shape is " +
                                                     ConvertDynamicShapeToString(inputs[i]));
                        concat_dim += inputs[i][iaxis].dim;
                     }
                     // other dimensions must be the same
                     else if (i > 0 && inputs[i][iaxis].GetVal() != inputs[i - 1][iaxis].GetVal())
                        throw std::runtime_error("TMVA SOFIE Concat Op - input tensors have wrong shapes " +
                                                 ConvertDynamicShapeToString(inputs[i]) + " and " +
                                                 ConvertDynamicShapeToString(inputs[i - 1]));
                  }
               }

               // output shape
               ret[0] = inputs[0];
               ret[0][fAxis].dim = concat_dim;
            }
            // case of stacking (not supported yet)
            // here we need to check that input shapes are the same
            // for example for fAxis == 0
            // output shapes: [inputs.size(), inputs[0][0], inputs[0][1],....]
            if(fnewAxis == 1){
               throw std::runtime_error("TMVA SOFIE Concat Op - stacking (i.e. COncatFromSequence with new_axis=1) is not supported ");
            }
            return ret;
         }

      void Initialize(RModel& model) override {
            for (auto &it : fInputs) {
               if (model.CheckIfTensorAlreadyExist(it) == false) {
                  throw std::runtime_error("TMVA SOFIE Concat Op Input Tensor " + it + " is not found in model");
               }
               fInputShapes.push_back(model.GetDynamicTensorShape(it));
            }
            fOutputShape = ShapeInference(fInputShapes)[0];
            if (model.Verbose())
               std::cout << "Output of concat operator has shape " << ConvertDynamicShapeToString(fOutputShape) << std::endl;

            // check if concat has constant inputs , axis 0(concat contigous memory and type is integer)
            if (model.GetTensorType(fInputs[0]) == ETensorType::INT64 && fAxis == 0) {
               fIsOutputConstant = true;
               for ( auto & input : fInputs) {
                  if (!model.IsInitializedTensor(input)) {
                     fIsOutputConstant = false;
                     break;
                  }
               }
               if (fIsOutputConstant) {
                  auto outputShape = ConvertShapeToInt(fOutputShape);  // conversion must be possible
                  std::vector<int64_t> outputData(ConvertShapeToLength(outputShape));
                  size_t offset = 0;
                  for ( auto & input : fInputs) {
                     auto inputData = static_cast<int64_t*>(model.GetInitializedTensorData(input).get());
                     auto inputShape = model.GetTensorShape(input); // shape is not dynamic if it is constant
                     size_t inputLength = ConvertShapeToLength(inputShape);
                     std::copy(inputData, inputData + inputLength, outputData.begin() + offset );
                     offset += inputLength;
                     // data do not need to be written as a weight
                     model.SetNotWritableInitializedTensor(input);
                  }
                  model.AddConstantTensor<int64_t>(fOutput, outputShape, outputData.data());
                  if (model.Verbose()) {
                     std::cout << "output of Concat is a constant tensor " << ConvertShapeToString(outputShape) << " : "
                     << ConvertValuesToString(outputData) << std::endl;
                  }
               }
            }
            if (!fIsOutputConstant) {
               model.AddIntermediateTensor(fOutput, model.GetTensorType(fInputs[0]), fOutputShape);
               if (model.Verbose()) {
                  std::cout << "Concat ---> " << fOutput << " " <<  ConvertDynamicShapeToString(fOutputShape) << std::endl;
               }
            }
         }

         std::string Generate(std::string OpName) override {
            if (fIsOutputConstant) return "";
            OpName = "op_"+OpName;
            if(fOutputShape.empty()){
                  throw std::runtime_error("TMVA SOFIE Concat called to Generate without being initialized first");
            }
            std::stringstream out;
            out<<"\n//--------- Concat\n";
            // special case when memory is contiguous
            bool hasShapeOnes = true;
            for(int i = 0; i<fAxis; ++i){
               if(fInputShapes[0][i].dim !=1){
                  hasShapeOnes = false;
                  break;
               }
            }
            if (fAxis == 0 || hasShapeOnes) {
               std::string offset;
               for(size_t i=0; i<fInputs.size(); ++i) {
                  std::string length = ConvertDynamicShapeToLength(fInputShapes[i]);
                  out << SP << "std::copy(tensor_" <<fInputs[i] << ", tensor_" <<fInputs[i] << "+" << length <<", tensor_"<<fOutput;
                  if (i > 0)  out << offset;
                  offset += " + " + length;
                  out << ");\n";
               }
            }
            else {

               std::vector<Dim> outStride = UTILITY::ComputeStrideFromShape(fOutputShape);
               std::vector<std::vector<Dim>> inStrides(fInputs.size());
               int idx = 0;
               for ( auto &s : inStrides) {
                  s = UTILITY::ComputeStrideFromShape(fInputShapes[idx]);
                  idx++;
               }
               for (int i = 0; i < fAxis; ++i) {
                  // loop on dimensions
                  out << SP << "for (size_t i" << i << " = 0; i" << i << " < " << fOutputShape[i].GetVal() << "; ++i" << i <<") {\n";
               }

               out << SP << SP << SP << "int idxOut = ";
               for (int k = 0; k < fAxis; k++) {
                  if (k > 0) out << " + ";
                  out << outStride[k].GetVal() << "*i" << k;
               }
               out << ";\n";

               for (size_t j = 0; j < fInputs.size(); j++) {
                  if (j>0)
                  out << SP << SP << SP << "idxOut += " << fInputShapes[j-1][fAxis].GetVal() << ";\n";
                  out << SP << SP << SP << "int idxIn" << j <<" = ";
                  for (int k = 0; k < fAxis; k++) {
                     if (k > 0) out << " + ";
                     out << inStrides[j][k].GetVal() << "*i" << k;
                  }
                  out << ";\n";
                  out << SP << SP << SP << "for (size_t iC = 0; iC < " << fInputShapes[j][fAxis].GetVal() << "; ++iC) {\n";
                  out << SP << SP << SP << SP << "tensor_" << fOutput << "[idxOut+iC] = tensor_" << fInputs[j] << "[idxIn" << j << "+iC];\n";
                  out << SP << SP << SP << "}\n";
               // concatenate the axis values
               }
                for (int i = 0; i < fAxis; ++i) {
                    out << SP << "}\n";
                }
            }

            return out.str();
         }
     };
 }//SOFIE

 #endif //SOFIE_ROPERATOR_CONCAT
