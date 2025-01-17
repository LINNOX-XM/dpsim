

#include <DPsim.h>
#include "../Examples.h"
#include <nlohmann/json.hpp>

using namespace DPsim;
using namespace CPS;
using namespace CPS::CIM;
using namespace Examples::Grids;
using namespace Examples::Components;
using json = nlohmann::json;
using syngenParametersKundur = Examples::Components::SynchronousGeneratorKundur::MachineParameters;

int main(int argc, char* argv[]) {	

	// ----- PARAMETRIZATION -----
	// General
	Real VnomMV = 24e3;
	Real VnomHV = 230e3;
	Real nomFreq = 60;
	Real ratio = VnomMV/VnomHV;
	Real nomOmega= nomFreq* 2*PI;

	// Synchronous generator 
	SynchronousGeneratorKundur::MachineParameters syngenKundur;
	Real setPointActivePower=300e6;
	Real setPointVoltage=1.05*VnomMV;

	// Breaker
	Real BreakerOpen = 1e9;
	Real BreakerClosed = 0.001;

	// Line
	// Parameters of HV line referred to MV side
	const CIGREHVAmerican::LineParameters lineCIGREHV;
	Real lineLength = 100;
	Real lineResistance = lineCIGREHV.lineResistancePerKm*lineLength*std::pow(ratio,2);
	Real lineInductance = lineCIGREHV.lineReactancePerKm*lineLength*std::pow(ratio,2)/nomOmega;
	Real lineCapacitance = lineCIGREHV.lineSusceptancePerKm*lineLength/std::pow(ratio,2)/nomOmega;
	Real lineConductance = 8e-2; //change to allow bigger time steps and to stabilize simulation (8e-2 used for 10us)

	// Simulation
	const Real startTimeFault=0.2;

	fs::path configFilename;
	
	CommandLineArgs args(argc, argv);
	if (args.params != "default.json")
		configFilename = args.params;
	else 
		configFilename = DPsim::Utils::findFile({
			"EMT_SynGenDQ7odTrapez_OperationalParams_SMIB_Fault_SyngenParams.json"
			}, "Configs/example_configs_json");


	std::ifstream jsonFile(configFilename);
	json simConfig = json::parse(jsonFile);
	const String simName = simConfig["name"].get<std::string>();


	// ----- POWERFLOW FOR INITIALIZATION -----
	String simNamePF = simName + "_PF";
	Logger::setLogDir("logs/" + simNamePF);

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
	simPF.setTimeStep(1.0);
	simPF.setFinalTime(2.0);
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
	// Synchronous generator
	auto gen = CPS::EMT::Ph3::SynchronGeneratorDQTrapez::make("SynGen", Logger::Level::debug);
	gen->setParametersOperationalPerUnit(
		syngenKundur.nomPower, syngenKundur.nomVoltage, syngenKundur.nomFreq, syngenKundur.poleNum, syngenKundur.nomFieldCurr,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, syngenKundur.H,
	 	initActivePower, initReactivePower, initTerminalVolt,
	 	initVoltAngle, syngenKundur.fieldVoltage, initMechPower
	); 
	DPsim::Utils::applySynchronousGeneratorParametersFromJson(simConfig, gen);
	
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
	sim.setTimeStep(10e-6);
	sim.setFinalTime(1.0);
	sim.setDomain(Domain::EMT);
	sim.setSystem(system);
	sim.addLogger(logger);
	sim.addEvent(sw1);
	sim.run();
}