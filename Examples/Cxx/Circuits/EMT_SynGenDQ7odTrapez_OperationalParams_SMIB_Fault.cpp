

#include <DPsim.h>
#include "../Examples.h"

using namespace DPsim;
using namespace CPS;
using namespace CPS::CIM;

int main(int argc, char* argv[]) {	

	// ----- PARAMETRIZATION -----
	// General grid parameters
	Real VnomMV = 24e3;
	Real VnomHV = 230e3;
	Real nomFreq = 60;
	Real ratio = VnomMV/VnomHV;
	Real nomOmega= nomFreq* 2*PI;

	// Machine parameters synchronous generator 
	const Examples::Components::SynchronousGeneratorKundur::MachineParameters syngenKundur;
	Real H = syngenKundur.H;
	Real Rs = syngenKundur.Rs;
	Real Ld = syngenKundur.Ld;
	Real Lq = syngenKundur.Lq;
	Real Ld_t = syngenKundur.Ld_t;
	Real Lq_t = syngenKundur.Lq_t;
	Real Ld_s = syngenKundur.Ld_s;
	Real Lq_s = syngenKundur.Lq_s;
	Real Ll = syngenKundur.Ll;
	Real Td0_t = syngenKundur.Td0_t;
	Real Tq0_t = syngenKundur.Tq0_t;
	Real Td0_s = syngenKundur.Td0_s;
	Real Tq0_s= syngenKundur.Tq0_s;	

	// Operation point synchronous generator 
	Real setPointActivePower=300e6;
	Real setPointVoltage=1.05*VnomMV;

	// Breaker
	Real BreakerOpen = 1e9;
	Real BreakerClosed = 0.001;

	// Line 
	const Examples::Grids::CIGREHVAmerican::LineParameters lineCIGREHV;
	Real lineLength = 100;
	Real lineResistance = lineCIGREHV.lineResistancePerKm*lineLength*std::pow(ratio,2); // HV parameters referred to MV side
	Real lineInductance = lineCIGREHV.lineReactancePerKm*lineLength*std::pow(ratio,2)/nomOmega;
	Real lineCapacitance = lineCIGREHV.lineSusceptancePerKm*lineLength/std::pow(ratio,2)/nomOmega;
	Real lineConductance = 8e-2; //change to allow bigger time steps and to stabilize simulation (8e-2 used for 10us)

	// Simulation parameters
	String simName = "EMT_SynGenDQ7odTrapez_OperationalParams_SMIB_Fault";
	Real finalTime = 1.0;
	Real timeStep = 10e-6;
	Real startTimeFault=0.2;

	// Command line args processing
	CommandLineArgs args(argc, argv);
	if (argc > 1) {

		// Simulation parameters
		if (args.name != "dpsim")
			simName = args.name;

		// Machine parameters
		if (args.options.find("H") != args.options.end())
			H = args.options["H"];
		if (args.options.find("Rs") != args.options.end())
			Rs = args.options["Rs"];
		if (args.options.find("Ld") != args.options.end())
			Ld = args.options["Ld"];
		if (args.options.find("Lq") != args.options.end())
			Lq = args.options["Lq"];
		if (args.options.find("Ld_t") != args.options.end())
			Ld_t = args.options["Ld_t"];
		if (args.options.find("Lq_t") != args.options.end())
			Lq_t = args.options["Lq_t"];
		if (args.options.find("Ld_s") != args.options.end())
			Ld_s = args.options["Ld_s"];
		if (args.options.find("Lq_s") != args.options.end())
			Lq_s = args.options["Lq_s"];
		if (args.options.find("Ll") != args.options.end())
			Ll = args.options["Ll"];
		if (args.options.find("Td0_t") != args.options.end())
			Td0_t = args.options["Td0_t"];
		if (args.options.find("Tq0_t") != args.options.end())
			Tq0_t = args.options["Tq0_t"];
		if (args.options.find("Td0_s") != args.options.end())
			Td0_s = args.options["Td0_s"];
		if (args.options.find("Tq0_s") != args.options.end())
			Tq0_s = args.options["Tq0_s"];
	}

	// ----- POWERFLOW FOR INITIALIZATION -----
	String simNamePF = simName + "_PF";
	Logger::setLogDir("logs/" + simNamePF);
	Real timeStepPF = finalTime;
	Real finalTimePF = finalTime+timeStepPF;

	// Components
	auto n1PF = SimNode<Complex>::make("n1", PhaseType::Single);
	auto n2PF = SimNode<Complex>::make("n2", PhaseType::Single);

	//Synchronous generator ideal model
	auto genPF = SP::Ph1::SynchronGenerator::make("Generator", Logger::Level::debug);
	genPF->setParameters(syngenKundur.nomPower, syngenKundur.nomVoltage, setPointActivePower, setPointVoltage, PowerflowBusType::PV);
	genPF->setBaseVoltage(VnomMV);
	genPF->modifyPowerFlowBusType(PowerflowBusType::PV);

	//Grid bus as Slack
	auto extnetPF = SP::Ph1::NetworkInjection::make("Slack", Logger::Level::debug);
	extnetPF->setParameters(VnomMV);
	extnetPF->setBaseVoltage(VnomMV);
	extnetPF->modifyPowerFlowBusType(PowerflowBusType::VD);
	
	//Line
	auto linePF = SP::Ph1::PiLine::make("PiLine", Logger::Level::debug);
	linePF->setParameters(lineResistance, lineInductance, lineCapacitance, lineConductance);
	linePF->setBaseVoltage(VnomMV);

	// Topology
	genPF->connect({ n1PF });
	linePF->connect({ n1PF, n2PF });
	extnetPF->connect({ n2PF });
	auto systemPF = SystemTopology(nomFreq,
			SystemNodeList{n1PF, n2PF},
			SystemComponentList{genPF, linePF, extnetPF});

	// Logging
	auto loggerPF = DataLogger::make(simNamePF);
	loggerPF->addAttribute("v1", n1PF->attribute("v"));
	loggerPF->addAttribute("v2", n2PF->attribute("v"));
	loggerPF->addAttribute("v_line", linePF->attribute("v_intf"));
	loggerPF->addAttribute("i_line", linePF->attribute("i_intf"));
	loggerPF->addAttribute("v_gen", genPF->attribute("v_intf"));
	loggerPF->addAttribute("ig", genPF->attribute("i_intf"));

	// Simulation
	Simulation simPF(simNamePF, Logger::Level::debug);
	simPF.setSystem(systemPF);
	simPF.setTimeStep(timeStepPF);
	simPF.setFinalTime(finalTimePF);
	simPF.setDomain(Domain::SP);
	simPF.setSolverType(Solver::Type::NRP);
	simPF.doInitFromNodesAndTerminals(false);
	simPF.addLogger(loggerPF);
	simPF.run();

	// ----- DYNAMIC SIMULATION ------	
	Logger::setLogDir("logs/"+simName);
	
	// Extract relevant powerflow results
	Real initTerminalVolt=std::abs(n1PF->singleVoltage())*RMS3PH_TO_PEAK1PH;
	Real initVoltAngle= Math::phase(n1PF->singleVoltage()); // angle in rad
	Real initActivePower = genPF->getApparentPower().real();
	Real initReactivePower = genPF->getApparentPower().imag();
	Real initMechPower = initActivePower;

	// Nodes	
	auto n1 = SimNode<Real>::make("n1", PhaseType::ABC);
	auto n2 = SimNode<Real>::make("n2", PhaseType::ABC);

	// Components
	//Synch
	auto gen = CPS::EMT::Ph3::SynchronGeneratorDQTrapez::make("SynGen", Logger::Level::debug);
	gen->setParametersOperationalPerUnit(
		syngenKundur.nomPower, syngenKundur.nomVoltage, syngenKundur.nomFreq, syngenKundur.poleNum, syngenKundur.nomFieldCurr,
		Rs, Ld, Lq, Ld_t, Lq_t, Ld_s,
		Lq_s, Ll, Td0_t, Tq0_t, Td0_s, Tq0_s, H,
	 	initActivePower, initReactivePower, initTerminalVolt,
	 	initVoltAngle, syngenKundur.fieldVoltage, initMechPower
	); 
	
	//Grid bus as Slack
	auto extnet = EMT::Ph3::NetworkInjection::make("Slack", Logger::Level::debug);

	// Line
	auto line = EMT::Ph3::PiLine::make("PiLine", Logger::Level::debug);
	line->setParameters(Math::singlePhaseParameterToThreePhase(lineResistance), 
	                      Math::singlePhaseParameterToThreePhase(lineInductance), 
					      Math::singlePhaseParameterToThreePhase(lineCapacitance),
						  Math::singlePhaseParameterToThreePhase(lineConductance));
				  
	//Breaker
	auto fault = CPS::EMT::Ph3::Switch::make("Br_fault", Logger::Level::debug);
	fault->setParameters(Math::singlePhaseParameterToThreePhase(BreakerOpen), 
						 Math::singlePhaseParameterToThreePhase(BreakerClosed));
	fault->openSwitch();

	// Topology
	gen->connect({ n1 });
	line->connect({ n1, n2 });
	extnet->connect({ n2 });
	fault->connect({EMT::SimNode::GND, n1});
	auto system = SystemTopology(nomFreq,
			SystemNodeList{n1, n2},
			SystemComponentList{gen, line, fault, extnet});

	// Initialization of dynamic topology
	CIM::Reader reader(simName, Logger::Level::debug);
	reader.initDynamicSystemTopologyWithPowerflow(systemPF, system);

	// Logging
	auto logger = DataLogger::make(simName);
	logger->addAttribute("v1", n1->attribute("v"));
	logger->addAttribute("v2", n2->attribute("v"));
	logger->addAttribute("v_line", line->attribute("v_intf"));
	logger->addAttribute("i_line", line->attribute("i_intf"));
	logger->addAttribute("v_gen", gen->attribute("v_intf"));
	logger->addAttribute("i_gen", gen->attribute("i_intf"));
	logger->addAttribute("wr_gen", gen->attribute("w_r"));
	logger->addAttribute("delta_r", gen->attribute("delta_r"));

	// Events
	auto sw1 = SwitchEvent3Ph::make(startTimeFault, fault, true);

	// Simulation
	Simulation sim(simName, Logger::Level::debug);
	sim.setSystem(system);
	sim.setTimeStep(timeStep);
	sim.setFinalTime(finalTime);
	sim.setDomain(Domain::EMT);
	sim.addLogger(logger);
	sim.addEvent(sw1);
	sim.run();
}