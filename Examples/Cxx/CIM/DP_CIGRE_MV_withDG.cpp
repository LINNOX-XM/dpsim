#include "cps/CIM/Reader.h"
#include <DPsim.h>
#include <cps/CSVReader.h>
#include "../Examples.h"

using namespace std;
using namespace DPsim;
using namespace CPS;
using namespace CPS::CIM;

int main(int argc, char** argv){

	// Simulation parameters
	String simName = "DP_CIGRE_MV_withDG";
	Examples::CIGREMV::ScenarioConfig scenario;
	std::list<fs::path> filenames;
	Real timeStep;
	Real finalTime;
	Bool steadyStateInit;
		
	// Set remaining simulation parameters using default values or command line infos
	std::cout<<std::experimental::filesystem::current_path()<<std::endl;
	CommandLineArgs args(argc, argv);
	if (argc <= 1) {
		filenames = DPsim::Utils::findFiles({
			"Rootnet_FULL_NE_28J17h_DI.xml",
			"Rootnet_FULL_NE_28J17h_EQ.xml",
			"Rootnet_FULL_NE_28J17h_SV.xml",
			"Rootnet_FULL_NE_28J17h_TP.xml"
		}, "dpsim/Examples/CIM/grid-data/CIGRE_MV/NEPLAN/CIGRE_MV_no_tapchanger_noLoad1_LeftFeeder_With_LoadFlow_Results", "CIMPATH");
		timeStep = 0.1e-3;
		finalTime = 1;
		steadyStateInit = false;
	}
	else {
		filenames = args.positionalPaths();
		timeStep = args.timeStep;
		finalTime = args.duration;
		steadyStateInit = args.steadyInit;
	}
	
	// ----- POWERFLOW FOR INITIALIZATION -----
	// read original network topology
	String simNamePF = simName + "_Powerflow";
	Logger::setLogDir("logs/" + simNamePF);
    CIM::Reader reader(simNamePF, Logger::Level::debug, Logger::Level::debug);
    SystemTopology systemPF = reader.loadCIM(scenario.systemFrequency, filenames, Domain::SP);
	Examples::CIGREMV::addInvertersToCIGREMV(systemPF, scenario, Domain::SP);

	// define logging
    auto loggerPF = DPsim::DataLogger::make(simNamePF);
    for (auto node : systemPF.mNodes)
    {
        loggerPF->addAttribute(node->name() + ".V", node->attribute("v"));
    }

	// run powerflow
    Simulation simPF(simNamePF, systemPF, 1, 2, Domain::SP, Solver::Type::NRP, Logger::Level::debug, true);
    simPF.addLogger(loggerPF);
    simPF.run();

	
	// ----- DYNAMIC SIMULATION -----
	Logger::setLogDir("logs/" + simName);
	CIM::Reader reader2(simName, Logger::Level::debug, Logger::Level::debug);
    SystemTopology systemDP = reader2.loadCIM(scenario.systemFrequency, filenames, CPS::Domain::DP);
	Examples::CIGREMV::addInvertersToCIGREMV(systemDP, scenario, Domain::DP);
	reader2.initDynamicSystemTopologyWithPowerflow(systemPF, systemDP);

	auto logger = DPsim::DataLogger::make(simName);

	// log node voltages
	for (auto node : systemDP.mNodes)
	{
		logger->addAttribute(node->name() + ".V", node->attribute("v"));
	}

	// log line currents
	for (auto comp : systemDP.mComponents) {
		if (dynamic_pointer_cast<CPS::DP::Ph1::PiLine>(comp))
			logger->addAttribute(comp->name() + ".I", comp->attribute("i_intf"));
	}

	// log load currents
	for (auto comp : systemDP.mComponents) {
		if (dynamic_pointer_cast<CPS::DP::Ph1::RXLoad>(comp))
			logger->addAttribute(comp->name() + ".I", comp->attribute("i_intf"));
	}

	// log output of PV connected at N11
	String pv_name = "pv_N11";
	auto pv = systemDP.component<CPS::SimPowerComp<Complex>>(pv_name);
	Examples::CIGREMV::logPVDecomposedAttributes(logger, pv);

	Simulation sim(simName, systemDP, timeStep, finalTime, Domain::DP, Solver::Type::MNA, Logger::Level::debug, true);

	sim.doSteadyStateInit(steadyStateInit);
	sim.addLogger(logger);
	sim.run();

}