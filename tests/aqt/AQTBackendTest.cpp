#include <dlfcn.h>
#include <iostream>
#include <memory>
#include <ostream>
#include <qdmi.h>
#include <qinfo.h>
#include <stdio.h>
#include <fstream>

#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/IR/Module.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <curl/curl.h>

using namespace llvm::orc;
using namespace llvm;


char* convertToQASM(Module& module){
    return NULL;
}



int main(){
    
    int err;
    QInfo info;
    QDMI_Session session;

    err = QInfo_create(&info);
    if QDMI_IS_ERROR(err) return err;
    
    err = QDMI_session_init(info, &session);
    if QDMI_IS_ERROR(err) return err;
    int count = -1;
    err = QDMI_core_device_count(&session, &count);
    if QDMI_IS_ERROR(err) return err;

    std::cout << count << " Device is available" << std::endl;

    QDMI_Device devices[count];
    for(int index = 0; index < count; index++)
        QDMI_core_open_device(&session, 0, &info, &devices[index]);

    QDMI_Device device = devices[0];
 
    char* backendName;
    err = QDMI_query_device_property_c(device, BACKEND_NAME, &backendName);
    if QDMI_IS_ERROR(err) return err;


    std::cout << "The backend name is " << backendName << std::endl;

    QDMI_Job quantumJob;
    QDMI_Fragment frag;

    const char *filename = "/home/ubuntu/backends/tests/inputs/basic_circuit.ll";
    std::ifstream file(filename, std::ios::binary);
    const std::streamsize chunkSize = 1024;
    char bufferQir[chunkSize];
    std::string genericQir;

    while (!file.eof())
    {
        file.read(bufferQir, chunkSize);
        genericQir.append(bufferQir, file.gcount());
    }
    file.close();

    ThreadSafeContext TSCtx(std::make_unique<LLVMContext>());
    SMDiagnostic error;
    std::unique_ptr<Module> module = parseIR(MemoryBufferRef(genericQir, "QIR (LRZ)"), error,
                      *TSCtx.getContext());

    ThreadSafeModule threadSafeModule = ThreadSafeModule(std::move(module), std::move(TSCtx));
    SmallVector<char, 0> buffer;
    std::string str;
    raw_string_ostream OS(str);
    void* qirmod;
    threadSafeModule.withModuleDo([&](Module &module){
        OS << module; 
        OS.flush();
        
        raw_svector_ostream ostream(buffer);
        WriteBitcodeToFile(module, ostream);
        
        qirmod = static_cast<void *>(buffer.data());
        err = QDMI_control_pack_qir(device, qirmod, &frag);

        if(err == QDMI_ERROR_NOTSUPPORTED){
            QDMI_control_pack_qasm2(device, convertToQASM(module), &frag);
        }

    });
    
    err = QDMI_control_submit(device, &frag,  1024, info, &quantumJob);
    if QDMI_IS_ERROR(err) return err;

    QDMI_Status status;
    err = QDMI_control_wait(device,  &quantumJob, &status);
    if QDMI_IS_ERROR(err) return err;


    int numbits;
    err = QDMI_control_readout_size(device, &status, quantumJob, &numbits);
    if QDMI_IS_ERROR(err) return err;


    int* raw_numbers = new int[1 << numbits];
    memset(raw_numbers, 0, sizeof(int) * (1 << numbits));
    QDMI_control_readout_raw_num(device, &status, quantumJob, raw_numbers);


    for(int i = 0; i < 1 << numbits; i++)
        std::cout << i << " : " << raw_numbers[i] << std::endl;




    free(raw_numbers);



}