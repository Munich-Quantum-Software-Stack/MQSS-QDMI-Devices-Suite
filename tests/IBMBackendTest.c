#include <qdmi.h>
#include <qinfo.h>
#include <stdio.h>


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

    
    QDMI_Device devices[count];
    for(int index = 0; index < count; index++)
        QDMI_core_open_device(&session, 0, &info, &devices[index]);

    QDMI_Device device = devices[0];

    char* backendName;
    err = QDMI_query_device_property_c(device, BACKEND_NAME, &backendName);
    if QDMI_IS_ERROR(err) return err;

    printf("The backend name is %s \n", backendName);
    //CHECK_ERR(err, "QInfo_create");


}