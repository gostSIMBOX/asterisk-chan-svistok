#ifndef SVISTOK_AT_COMMAND_H_INCLUDED
#define SVISTOK_AT_COMMAND_H_INCLUDED

EXPORT_DECL int at_enque_cfun1 (struct cpvt * cpvt);
EXPORT_DECL int at_enque_cfun5 (struct cpvt * cpvt);
EXPORT_DECL int at_enque_cfun6 (struct cpvt * cpvt);
EXPORT_DECL int at_enque_cfun_v (struct cpvt * cpvt);
EXPORT_DECL int at_enque_cmd_proc (struct cpvt * cpvt, const char* cmd);
EXPORT_DECL int at_enque_cpin_v (struct cpvt * cpvt);
EXPORT_DECL int at_enque_iccid (struct cpvt * cpvt);
EXPORT_DECL int at_enque_initialization_modem(struct cpvt* cpvt);
EXPORT_DECL int at_enque_initialization_sim(struct cpvt* cpvt);
EXPORT_DECL int at_enque_initialization_sim_e(struct cpvt * cpvt);
EXPORT_DECL int at_enque_initialization_sim_mb(struct cpvt * cpvt);
EXPORT_DECL int at_enque_sn (struct cpvt * cpvt);
EXPORT_DECL int at_enque_spn (struct cpvt * cpvt);
EXPORT_DECL int at_enque_sysinfo (struct cpvt * cpvt);

#endif /* SVISTOK_AT_COMMAND_H_INCLUDED */
