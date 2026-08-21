

typedef struct call_request
{
    const char * resource;				// для CMD параметров
    const struct ast_channel * requestor;		// для ENV
    const struct public_state * state;
    int opts;

    int dial_type; // 1 - gruppovoy tipa L0<=HZ

    int group;
    int limitnum;
    char limittype;
    char alg; //depreteated soon, for compatibility

    char numberdial[32];
    char numberb[32];
    char numbera[32];


    char napr_str[16];
    int billing_pay;				// Оплачиваемый ли
    char billing_direction[3];			//2 буквы направлния

    char  IMB_str[2048];				// imsi с которых на этот номер B звонили
    char  IMB_buf[2048];				// buf
    char* IMB[70];				// Отпарсенный.
    int   IMB_count;				// Количество
    int   IMB_any;				// 1 - можно с любого, 0 - только из списка

    char qos_str[16];
    char cap_str[16];
    char spec_str[16];

    char pre_str[16];
    char pos_str[16];
    char pro_str[16];
    
    char diff_penalty_str[16];

    int qos;
    int cap;
    int spec;
    int pre;
    int pos;
    int diff_penalty;

    int fas;
    int epdd;
    int fpdd;
    int hem;


    char souimsito[64];

    char uid[64];
} call_request_t;


typedef struct pvt_select
{
    struct pvt*pvt;
    int IMB_pos;
    char IMB_type;
//    int uu_im;
//    int uu_qos;
//    int ok;
    int diff;
    long sou_diff_start;

} pvt_select_t;


void pvt_select_stat(struct call_request *cr, struct pvt_select * pvt_select);