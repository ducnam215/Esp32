#ifndef Register_bien_tan_h
#define Register_bien_tan_h

#define Run 1
#define Stop 0

#define Reverse Run
#define Forward Stop

// ModBus Coil List
#define Operation_command 0x0001U
#define Rotation_direction_command 0x0002U
#define Operation_status 0x000FU
#define Rotation_direction 0x0010U
#define Inverter_ready 0x0011U

// ModBus Holding Registers
#define Frequency_source 0x0001U
#define Output_frequency_monitor 0x1001U
#define Output_current_monitor 0x1003U
#define Rotation_direction_minitoring 0x1004U
#define Output_voltage_monitor 0x1011U
#define Power_monitor 0x1012U

#endif /*Register_bien_tan_h*/