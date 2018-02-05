/*
 * adpcontrol.cpp
 *
 * Created: 30/01/2018 5:15:20 PM
 *  Author: teddy
 */ 

#include "adpcontrol.h"
#include "dep_instance.h"
#include <string.h>

extern "C" {

usart_module instance;
SemaphoreHandle_t sem_recvComplete;
uint8_t usart_response_buf[2 + ADP_LENGTH_PACKET_HEADER];

void adp_interface_transceive_procotol(uint8_t* tx_buf, uint16_t length, uint8_t* rx_buf) {
	usart_write_buffer_job(&instance, tx_buf, length);
	usart_read_buffer_job(&instance, usart_response_buf, sizeof usart_response_buf);
}

status_code adp_interface_read_response(uint8_t *rx_data, uint16_t length) {
	if(xSemaphoreTake(sem_recvComplete, msToTicks(10)) == pdFALSE)
		return STATUS_ERR_TIMEOUT;
	memcpy(rx_data, usart_response_buf, sizeof usart_response_buf);
	return STATUS_OK;
	//return usart_read_buffer_wait(&instance, rx_data, length);
}

void recv_complete_callback(usart_module *const module) {
	BaseType_t prio = 0;
	xSemaphoreGiveFromISR(sem_recvComplete, &prio);
	portYIELD_FROM_ISR(prio);
}

status_code adp_interface_init(void) {
	//This is just copied from ManualSerial, should just put it in a function
	usart_config config;
	usart_get_config_defaults(&config);
	config.baudrate = 57600;
	config.character_size = USART_CHARACTER_SIZE_8BIT;
	config.clock_polarity_inverted = false;
	config.collision_detection_enable = false;
	config.data_order = USART_DATAORDER_LSB;
	config.encoding_format_enable = false;
	config.generator_source = GCLK_GENERATOR_1;
	config.mux_setting = USART_RX_1_TX_0_XCK_1;
	config.parity = USART_PARITY_NONE;
	config.pinmux_pad0 = PINMUX_PB02D_SERCOM5_PAD0;
	config.pinmux_pad1 = PINMUX_PB03D_SERCOM5_PAD1;
	config.pinmux_pad2 = PINMUX_UNUSED;
	config.pinmux_pad3 = PINMUX_UNUSED;
	config.receiver_enable = true;
	config.sample_rate = USART_SAMPLE_RATE_8X_ARITHMETIC;
	//Might be supposed to be true
	config.start_frame_detection_enable = false;
	config.stopbits = USART_STOPBITS_1;
	config.transfer_mode = USART_TRANSFER_ASYNCHRONOUSLY;
	config.transmitter_enable = true;
	config.use_external_clock = false;
	usart_init(&instance, SERCOM5, &config);
	usart_register_callback(&instance, recv_complete_callback, USART_CALLBACK_BUFFER_RECEIVED);
	usart_enable_callback(&instance, USART_CALLBACK_BUFFER_RECEIVED);

	usart_enable(&instance);
	return STATUS_OK;
}

}

struct StreamID {
	enum e {
		 Time = 0,
		 Voltage,
		 Motor0DutyCycle,
		 Motor1DutyCucle,
		 Servo0Position,
		 Servo1Position,
		 Motor0Speed,
		 Motor1Speed,
		 DriveSpeed,
		 VehicleSpeed,
		 Motor0Current,
		 Motor1Current,
		 Motor0EnergyUsage,
		 Motor1EnergyUsage,
		 TotalEnergyUsage,
		 Distace,
		 OPState,
		 CurrentTaskString,
		 _size,
	};
};

//Elements
struct ElementID {
	enum e {
		Graph_GearsPerSecond = 0,
		Graph_DutyCycle,
		Graph_ServoPosition,
		Label_TimeLabel,
		Label_Time,
		Label_VehicleSpeedLabel,
		Label_VehicleSpeed,
		Label_DistanceLabel,
		Label_Distance,
		Label_CurrentTaskLabel,
		Label_CurrentTask,
		_size,
	};
};

struct GraphID {
	enum e {
		VehicleSpeed = 0,
	};
};

static char const *streamNames[StreamID::_size] = {
	"Time",
	"Voltage",
	"Motor0DutyCycle",
	"Motor1DutyCycle",
	"Servo0Position",
	"Servo1Position",
	"Motor0Speed",
	"Motor1Speed",
	"DriveSpeed",
	"VehicleSpeed",
	"Motor0Current",
	"Motor1Current",
	"Motor0EnergyUsage",
	"Motor1EnergyUsage",
	"TotalEnergyUsage",
	"Distance",
	"OPState",
	"CurrentTaskString"
};

static char const *taskNames[static_cast<unsigned int>(TaskIdentity::_size)] = {
	"None",
	"Idle",
	"Startup",
	"Engage",
	"Disengage",
	"Coast",
	"CoastRamp",
	"SpeedMatch",
	"OPCheck",
	"BatteryCheck",
	"MotorCheck",
	"ADPControl"
};

void ADPControl::init()
{
	adp_init();
	//Create semaphrore
	if((sem_recvComplete = xSemaphoreCreateBinary()) == NULL)
		debugbreak();
	//Start task
	if(xTaskCreate(taskFunction, config::adpcontrol::taskName, config::adpcontrol::taskStackDepth, this, config::adpcontrol::taskPriority, &task) == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
		debugbreak();
}

Run::Output ADPControl::update(Input const &input)
{
	localinput = input;
	localinput.vehicleSpeed = msToKmh(localinput.vehicleSpeed);
	localinput.motor0Current = input.bms0data.cellVoltage[0];
	localinput.motor1Current = input.bms0data.current;
	xTaskNotifyGive(task);
	return {};
}

Run::Task * ADPControl::complete(Input const &input)
{
	return nullptr;
}

ADPControl::ADPControl() : Task(TaskIdentity::ADPControl)
{
}

void ADPControl::taskFunction(void *const adpcontrol)
{
	static_cast<ADPControl *>(adpcontrol)->task_main();
}

void ADPControl::fillInputAddresses(void *values[])
{
	values[StreamID::Time				] = &localinput.time;
	values[StreamID::Voltage			] = &localinput.voltage;
	values[StreamID::Motor0DutyCycle	] = &localinput.motor0DutyCycle;
	values[StreamID::Motor1DutyCucle	] = &localinput.motor1DutyCycle;
	values[StreamID::Servo0Position		] = &localinput.servo0Position;
	values[StreamID::Servo1Position		] = &localinput.servo1Position;
	values[StreamID::Motor0Speed		] = &localinput.motor0Speed;
	values[StreamID::Motor1Speed		] = &localinput.motor1Speed;
	values[StreamID::DriveSpeed			] = &localinput.driveSpeed;
	values[StreamID::VehicleSpeed		] = &localinput.vehicleSpeed;
	values[StreamID::Motor0Current		] = &localinput.motor0Current;
	values[StreamID::Motor1Current		] = &localinput.motor1Current;
	values[StreamID::Motor0EnergyUsage	] = &localinput.motor0EnergyUsage;
	values[StreamID::Motor1EnergyUsage	] = &localinput.motor1EnergyUsage;
	values[StreamID::TotalEnergyUsage	] = &localinput.totalEnergyUsage;
	values[StreamID::Distace			] = &localinput.distance;
	values[StreamID::OPState			] = &localinput.opState;
}

void ADPControl::task_main()
{
	adp_wait_for_handshake();
	//Setup streams
	for(unsigned int i = 0; i < StreamID::_size; i++) {
		adp_msg_configure_stream stream;
		adp_configure_stream_get_defaults(&stream);
		stream.stream_id = i;
		if(i == StreamID::OPState)
			stream.type = ADP_STREAM_BOOL;
		else if (i == StreamID::CurrentTaskString)
			stream.type = ADP_STREAM_STRING;
		else
			stream.type = ADP_STREAM_FLOAT;
		stream.state = ADP_STREAM_ON;
		stream.mode = ADP_STREAM_OUT;
		adp_configure_stream(&stream, streamNames[i]);
	}

	//Setup dashboard and graph
	setupDashboard();
	setupGraph();

	//Blink red LED 3 times to indicate that ADP has been setup
	runtime::redLEDmanager->registerSequence([](LED &l) {
		for(unsigned int i = 0; i < 3; i++) {
			l.setLEDState(true);
			vTaskDelay(msToTicks(100));
			l.setLEDState(false);
			vTaskDelay(msToTicks(100));
		}
	});
	
	adp_msg_data_stream outs;
	void *dataAddresses[StreamID::CurrentTaskString];
	fillInputAddresses(dataAddresses);
	while(true) {
		ulTaskNotifyTake(0, portMAX_DELAY);
		outs.number_of_streams = 0;
		for(unsigned int i = 0; i < StreamID::CurrentTaskString; i++) {
			outs.stream[i].stream_id = i;;
			outs.stream[i].data_size = (i == StreamID::OPState) ? sizeof(bool) : sizeof(float);
			outs.stream[i].data = reinterpret_cast<uint8_t *>(dataAddresses[i]);
			outs.number_of_streams++;
		}
		if(localinput.currentID != lastTask) {
			outs.stream[StreamID::CurrentTaskString].stream_id = StreamID::CurrentTaskString;
			outs.stream[StreamID::CurrentTaskString].data_size = strlen(taskNames[static_cast<unsigned int>(localinput.currentID)]);
			//Have to strcpy into a new buf since I'm too stubborn to use const_cast
			char namebuf[ADP_CONF_MAX_LABEL];
			strcpy(namebuf, taskNames[static_cast<unsigned int>(localinput.currentID)]);
			outs.stream[StreamID::CurrentTaskString].data = reinterpret_cast<uint8_t *>(namebuf);
			outs.number_of_streams++;
		}
		lastTask = localinput.currentID;
		//Send streams
		uint8_t recvbuf[MSG_RES_DATA_MAX_LEN];
		adp_transceive_stream(&outs, recvbuf);
	}
}

void ADPControl::setupDashboard()
{
	//Add dasbboard
	adp_msg_conf_dashboard dashboard;
	adp_conf_dashboard_get_defaults(&dashboard);
	adp_set_color(dashboard.color, ADP_COLOR_WHITE);
	dashboard.height = 450;
	adp_add_dashboard(&dashboard, "Dashboard");

	adp_msg_conf_dashboard_element_graph graph;

	//Setup gears per second graph
	adp_conf_dashboard_graph_get_defaults(&graph);
	graph.dashboard_id = 0;
	graph.element_id = ElementID::Graph_GearsPerSecond;
	graph.x = 0;
	graph.y = 0;
	graph.width = 320;
	graph.height = 240;
	graph.plot_count = 3;
	graph.y_max = 2000;
	graph.mode.bit.fit_graph = 0;
	graph.mode.bit.mouse = 0;
	adp_add_graph_to_dashboard(&graph, "Gears per second");

	//Setup duty cycle graph
	adp_conf_dashboard_graph_get_defaults(&graph);
	graph.dashboard_id = 0;
	graph.element_id = ElementID::Graph_DutyCycle;
	graph.x = 1 * (320 + 1);
	graph.y = 0;
	graph.width = 320;
	graph.height = 240;
	graph.plot_count = 2;
	graph.y_max = 1;
	graph.mode.bit.fit_graph = 0;
	graph.mode.bit.mouse = 0;
	adp_add_graph_to_dashboard(&graph, "Duty Cycle");

	//Setup duty cycle graph
	adp_conf_dashboard_graph_get_defaults(&graph);
	graph.dashboard_id = 0;
	graph.element_id = ElementID::Graph_ServoPosition;
	graph.x = 2 * (320 + 1);
	graph.y = 0;
	graph.width = 320;
	graph.height = 240;
	graph.plot_count = 2;
	graph.y_min = -180;
	graph.y_max = 180;
	graph.mode.bit.fit_graph = 0;
	graph.mode.bit.mouse = 0;
	adp_add_graph_to_dashboard(&graph, "Servo Position");

	adp_msg_conf_dashboard_element_label label;
	
	//Setup time label ("Time:")
	adp_conf_dashboard_label_get_defaults(&label);
	label.dashboard_id = 0;
	label.element_id = ElementID::Label_TimeLabel;
	label.x = 10;
	label.y = 322;
	label.width = 112;
	label.height = 16;
	label.font_size = 12;
	label.horisontal_alignment = HORISONTAL_ALIGNMENT_RIGHT;
	adp_set_color(label.background_color, ADP_COLOR_WHITE);
	label.foreground_transparency = 0xff;
	adp_add_label_to_dashboard(&label, "Time:");

	//Setup time numeric label ("<a number>")
	adp_conf_dashboard_label_get_defaults(&label);
	label.dashboard_id = 0;
	label.element_id = ElementID::Label_Time;
	label.x = 130;
	label.y = 322;
	label.width = 64;
	label.height = 16;
	label.font_size = 12;
	adp_set_color(label.background_color, ADP_COLOR_WHITE);
	label.foreground_transparency = 0xff;
	adp_add_label_to_dashboard(&label, "time");

	//Setup vehicle speed label
	adp_conf_dashboard_label_get_defaults(&label);
	label.dashboard_id = 0;
	label.element_id = ElementID::Label_VehicleSpeedLabel;
	label.x = 10;
	label.y = 322 + (16 * 1);
	label.width = 112;
	label.height = 16;
	label.font_size = 12;
	label.horisontal_alignment = HORISONTAL_ALIGNMENT_RIGHT;
	adp_set_color(label.background_color, ADP_COLOR_WHITE);
	label.foreground_transparency = 0xff;
	adp_add_label_to_dashboard(&label, "Vehicle Speed (kmh):");

	//Setup vehicle speed numeric label
	adp_conf_dashboard_label_get_defaults(&label);
	label.dashboard_id = 0;
	label.element_id = ElementID::Label_VehicleSpeed;
	label.x = 130;
	label.y = 322 + (16 * 1);
	label.width = 64;
	label.height = 16;
	label.font_size = 12;
	adp_set_color(label.background_color, ADP_COLOR_WHITE);
	label.foreground_transparency = 0xff;
	adp_add_label_to_dashboard(&label, "speed");

	//Setup distance label
	adp_conf_dashboard_label_get_defaults(&label);
	label.dashboard_id = 0;
	label.element_id = ElementID::Label_DistanceLabel;
	label.x = 10;
	label.y = 322 + (16 * 2);
	label.width = 112;
	label.height = 16;
	label.font_size = 12;
	label.horisontal_alignment = HORISONTAL_ALIGNMENT_RIGHT;
	adp_set_color(label.background_color, ADP_COLOR_WHITE);
	label.foreground_transparency = 0xff;
	adp_add_label_to_dashboard(&label, "Distance (km):");

	//Setup distance numeric label
	adp_conf_dashboard_label_get_defaults(&label);
	label.dashboard_id = 0;
	label.element_id = ElementID::Label_Distance;
	label.x = 130;
	label.y = 322 + (16 * 2);
	label.width = 64;
	label.height = 16;
	label.font_size = 12;
	adp_set_color(label.background_color, ADP_COLOR_WHITE);
	label.foreground_transparency = 0xff;
	adp_add_label_to_dashboard(&label, "distance");

	//Setup current task label
	adp_conf_dashboard_label_get_defaults(&label);
	label.dashboard_id = 0;
	label.element_id = ElementID::Label_CurrentTaskLabel;
	label.x = 10;
	label.y = 322 + (16 * 3);
	label.width = 112;
	label.height = 16;
	label.font_size = 12;
	label.horisontal_alignment = HORISONTAL_ALIGNMENT_RIGHT;
	adp_set_color(label.background_color, ADP_COLOR_WHITE);
	label.foreground_transparency = 0xff;
	adp_add_label_to_dashboard(&label, "Current Task:");

	//Setup vehicle speed numeric label
	adp_conf_dashboard_label_get_defaults(&label);
	label.dashboard_id = 0;
	label.element_id = ElementID::Label_CurrentTask;
	label.x = 130;
	label.y = 322 + (16 * 3);
	label.width = 64;
	label.height = 16;
	label.font_size = 12;
	adp_set_color(label.background_color, ADP_COLOR_WHITE);
	label.foreground_transparency = 0xff;
	adp_add_label_to_dashboard(&label, "currenttask");

	//Add streams to elements
	adp_conf_add_stream_to_element link;
	link.dashboard_id = 0;
	link.element_id = ElementID::Graph_GearsPerSecond;
	link.stream_id = StreamID::Motor0Speed;
	adp_add_stream_to_element(&link);

	link.dashboard_id = 0;
	link.element_id = ElementID::Graph_GearsPerSecond;
	link.stream_id = StreamID::Motor1Speed;
	adp_add_stream_to_element(&link);

	link.dashboard_id = 0;
	link.element_id = ElementID::Graph_GearsPerSecond;
	link.stream_id = StreamID::DriveSpeed;
	adp_add_stream_to_element(&link);

	link.dashboard_id = 0;
	link.element_id = ElementID::Graph_DutyCycle;
	link.stream_id = StreamID::Motor0DutyCycle;
	adp_add_stream_to_element(&link);

	link.dashboard_id = 0;
	link.element_id = ElementID::Graph_DutyCycle;
	link.stream_id = StreamID::Motor1DutyCucle;
	adp_add_stream_to_element(&link);

	link.dashboard_id = 0;
	link.element_id = ElementID::Graph_ServoPosition;
	link.stream_id = StreamID::Servo0Position;
	adp_add_stream_to_element(&link);

	link.dashboard_id = 0;
	link.element_id = ElementID::Graph_ServoPosition;
	link.stream_id = StreamID::Servo1Position;
	adp_add_stream_to_element(&link);

	link.dashboard_id = 0;
	link.element_id = ElementID::Label_Time;
	link.stream_id = StreamID::Time;
	adp_add_stream_to_element(&link);

	link.dashboard_id = 0;
	link.element_id = ElementID::Label_VehicleSpeed;
	link.stream_id = StreamID::VehicleSpeed;
	adp_add_stream_to_element(&link);

	link.dashboard_id = 0;
	link.element_id = ElementID::Label_Distance;
	link.stream_id = StreamID::Distace;
	adp_add_stream_to_element(&link);

	link.dashboard_id = 0;
	link.element_id = ElementID::Label_CurrentTask;
	link.stream_id = StreamID::CurrentTaskString;
	adp_add_stream_to_element(&link);
}

void ADPControl::setupGraph()
{
	struct AxisID {
		enum e {
			VehicleSpeed,
		};
	};
	//Setup vehicle speed graph
	adp_msg_configure_graph graph;
	adp_configure_graph_get_defaults(&graph);
	graph.graph_id = GraphID::VehicleSpeed;
	graph.scale_mode = ADP_GRAPH_SCALE_OFF;
	graph.scroll_mode = ADP_GRAPH_SCROLL_SCROLL;
	adp_set_color(graph.background_color, ADP_COLOR_BLACK);
	adp_configure_graph(&graph, "Vehicle Speed", "time");

	adp_msg_conf_axis axis;
	adp_add_axis_to_graph_get_defaults(&axis);
	axis.axis_id = AxisID::VehicleSpeed;
	axis.graph_id = GraphID::VehicleSpeed;
	axis.y_min = 0;
	axis.y_max = 50;
	adp_set_color(axis.color, ADP_COLOR_BLACK);
	adp_add_axis_to_graph(&axis, "Vehicle Speed");

	adp_msg_add_stream_to_axis link; //HIYAHHH!

	adp_add_stream_to_axis_get_defaults(&link);
	link.graph_id = GraphID::VehicleSpeed;
	link.axis_id = AxisID::VehicleSpeed;
	link.stream_id = StreamID::CurrentTaskString;
	//link.y_scale_numerator = 0;
	//link.y_scale_denominator = 50;
	link.transparency = 255;
	link.mode = 0x3;
	link.line_thickness = 2;
	adp_set_color(link.line_color, ADP_COLOR_GREEN);
	adp_add_stream_to_axis(&link);

	adp_add_stream_to_axis_get_defaults(&link);
	link.graph_id = GraphID::VehicleSpeed;
	link.axis_id = AxisID::VehicleSpeed;
	link.stream_id = StreamID::VehicleSpeed;
	//link.y_scale_numerator = 0;
	//link.y_scale_denominator = 50;
	link.transparency = 255;
	link.mode = ADP_AXIS_LINE_bm;
	link.line_thickness = 1;
	adp_set_color(link.line_color, ADP_COLOR_BLACK);
	adp_add_stream_to_axis(&link);
}
