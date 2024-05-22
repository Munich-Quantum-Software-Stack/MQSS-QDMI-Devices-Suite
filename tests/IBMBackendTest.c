#include <qdmi.h>
#include <qinfo.h>


int main(){
    int err;
    QInfo info;
    QDMI_Session session;

    err = QInfo_create(&info);
    if(err != QDMI_SUCCESS){
        return err;
    }
    
    err = QDMI_session_init(info, &session);

    
    //CHECK_ERR(err, "QInfo_create");


}