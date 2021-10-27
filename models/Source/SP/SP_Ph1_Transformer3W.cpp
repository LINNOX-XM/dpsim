/* Copyright 2017-2020 Institute for Automation of Complex Power Systems,
 *                     EONERC, RWTH Aachen University
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *********************************************************************************/

#include <cps/SP/SP_Ph1_Transformer3W.h>

using namespace CPS;

// #### General ####
SP::Ph1::Transformer3W::Transformer3W(String uid, String name, Logger::Level logLevel, Bool withResistiveLosses)
	: SimPowerComp<Complex>(uid, name, logLevel) {
	if (withResistiveLosses)
		setVirtualNodeNumber(7);
	else
		setVirtualNodeNumber(4);

	mSLog->info("Create {} {}", this->type(), name);
	mIntfVoltage = MatrixComp::Zero(3, 1);
	mIntfCurrent = MatrixComp::Zero(3, 1);
	setTerminalNumber(3);

	addAttribute<Real>("nominal_voltage_end1", &mNominalVoltageEnd1, Flags::read | Flags::write);
	addAttribute<Real>("nominal_voltage_end2", &mNominalVoltageEnd2, Flags::read | Flags::write);
	addAttribute<Real>("nominal_voltage_end3", &mNominalVoltageEnd3, Flags::read | Flags::write);
	addAttribute<Real>("base_voltage", &mBaseVoltage, Flags::read | Flags::write);
	addAttribute<Real>("S1", &mRatedPower1, Flags::write | Flags::read);
	addAttribute<Real>("S2", &mRatedPower2, Flags::write | Flags::read);
	addAttribute<Real>("S3", &mRatedPower3, Flags::write | Flags::read);
	addAttribute<Complex>("ratio1", &mRatio1, Flags::write | Flags::read);
	addAttribute<Complex>("ratio2", &mRatio2, Flags::write | Flags::read);
	addAttribute<Complex>("ratio3", &mRatio3, Flags::write | Flags::read);
	addAttribute<Real>("R1", &mResistance1, Flags::write | Flags::read);
	addAttribute<Real>("R2", &mResistance2, Flags::write | Flags::read);
	addAttribute<Real>("R3", &mResistance3, Flags::write | Flags::read);
	addAttribute<Real>("L1", &mInductance1, Flags::write | Flags::read);
	addAttribute<Real>("L2", &mInductance2, Flags::write | Flags::read);
	addAttribute<Real>("L3", &mInductance3, Flags::write | Flags::read);
	addAttribute<Bool>("nodal_injection_stored", &mStoreNodalPowerInjection, Flags::read | Flags::write);
	addAttribute<Real>("p_inj", &mActivePowerInjection, Flags::read | Flags::write);
	addAttribute<Real>("q_inj", &mReactivePowerInjection, Flags::read | Flags::write);
	addAttribute<Complex>("current_1", &mCurrent(0), Flags::read | Flags::write);
	addAttribute<Complex>("current_2", &mCurrent(1), Flags::read | Flags::write);
	addAttribute<Complex>("current_3", &mCurrent(2), Flags::read | Flags::write);
	addAttribute<Real>("p_branch_1", &mActivePowerBranch(0), Flags::read | Flags::write);
	addAttribute<Real>("q_branch_1", &mReactivePowerBranch(0), Flags::read | Flags::write);
	addAttribute<Real>("p_branch_2", &mActivePowerBranch(1), Flags::read | Flags::write);
	addAttribute<Real>("q_branch_2", &mReactivePowerBranch(1), Flags::read | Flags::write);
	addAttribute<Real>("p_branch_3", &mActivePowerBranch(2), Flags::read | Flags::write);
	addAttribute<Real>("q_branch_3", &mReactivePowerBranch(2), Flags::read | Flags::write);
}


void SP::Ph1::Transformer3W::setParameters(
		Real nomVoltageEnd1, Real nomVoltageEnd2, Real nomVoltageEnd3, 
		Real ratioAbs1, Real ratioAbs2, Real ratioAbs3, 
		Real ratioPhase1, Real ratioPhase2, Real ratioPhase3, 
		Real resistance1, Real resistance2, Real resistance3,
		Real inductance1, Real inductance2, Real inductance3
	) {

	// Note: to be consistent impedance values must be referred to high voltage side (and base voltage set to higher voltage)
	Base::Ph1::Transformer3W::setParameters(
			nomVoltageEnd1, nomVoltageEnd2, nomVoltageEnd3, 
			ratioAbs1, ratioAbs2, ratioAbs3, 
			ratioPhase1, ratioPhase2, ratioPhase3,
			resistance1, resistance2, resistance3, 
			inductance1, inductance2, inductance3
		);
	
	mSLog->info("Nominal Voltage End 1={} [V] Nominal Voltage End 2={} [V] Nominal Voltage End 2={} [V]", 
				mNominalVoltageEnd1, mNominalVoltageEnd2, mNominalVoltageEnd3);
	mSLog->info("Resistance={} [Ohm] Inductance={} [H] (primary side)", mResistance1, mInductance1);
	mSLog->info("Resistance={} [Ohm] Inductance={} [H] (secondary side)", mResistance2, mInductance2);
	mSLog->info("Resistance={} [Ohm] Inductance={} [H] (tetrary side)", mResistance3, mInductance3);
    mSLog->info("Tap Ratio 1={} [/] Phase Shift 1={} [deg]", std::abs(mRatio1), std::arg(mRatio1));
	mSLog->info("Tap Ratio 2={} [/] Phase Shift 2={} [deg]", std::abs(mRatio2), std::arg(mRatio2));
	mSLog->info("Tap Ratio 3={} [/] Phase Shift 3={} [deg]", std::abs(mRatio3), std::arg(mRatio3));

	mRatioAbs1 = std::abs(mRatio1);
	mRatioAbs2 = std::abs(mRatio2);
	mRatioAbs3 = std::abs(mRatio3);
	mRatioPhase1 = std::arg(mRatio1);
	mRatioPhase2 = std::arg(mRatio2);
	mRatioPhase3 = std::arg(mRatio3);
	mConductance1 = 1 / mResistance1;
	mConductance2 = 1 / mResistance2;
	mConductance3 = 1 / mResistance3;

	mParametersSet = true;
}

void SP::Ph1::Transformer3W::setParameters(
	Real nomVoltageEnd1, Real nomVoltageEnd2, Real nomVoltageEnd3,
	Real ratedPower1, Real ratedPower2, Real ratedPower3,
	Real ratioAbs1, Real ratioAbs2, Real ratioAbs3, 
	Real ratioPhase1, Real ratioPhase2, Real ratioPhase3, 
	Real resistance1, Real resistance2, Real resistance3,
	Real inductance1, Real inductance2, Real inductance3
	) {

	mRatedPower1 = ratedPower1;
	mRatedPower2 = ratedPower2;
	mRatedPower3 = ratedPower3;
	mSLog->info("Rated Power ={} [W]", mRatedPower1);
	mSLog->info("Rated Power ={} [W]", mRatedPower2);
	mSLog->info("Rated Power ={} [W]", mRatedPower3);

	SP::Ph1::Transformer3W::setParameters(
		nomVoltageEnd1, nomVoltageEnd2, nomVoltageEnd3, 
		ratioAbs1, ratioAbs2, ratioAbs3, 
		ratioPhase1, ratioPhase2, ratioPhase3, 
		resistance1, resistance2, resistance3,
		inductance1, inductance2, inductance3);
}


SimPowerComp<Complex>::Ptr SP::Ph1::Transformer3W::clone(String name) {
	auto copy = Transformer3W::make(name, mLogLevel);
	copy->setParameters(
			mNominalVoltageEnd1, mNominalVoltageEnd2, mNominalVoltageEnd3,
			mRatedPower1, mRatedPower2, mRatedPower3,
			std::abs(mRatio1), std::abs(mRatio2), std::abs(mRatio3), 
			std::arg(mRatio1), std::arg(mRatio2), std::arg(mRatio3), 
			mResistance1, mResistance2, mResistance3,
			mInductance1, mInductance2, mInductance3
		);
	return copy;
}

void SP::Ph1::Transformer3W::initializeFromNodesAndTerminals(Real frequency) {
	mNominalOmega = 2. * PI * frequency;
	mReactance1 = mNominalOmega * mInductance1;
	mReactance2 = mNominalOmega * mInductance2;
	mReactance3 = mNominalOmega * mInductance3;
	mSLog->info("Reactance={} [Ohm] (primary side)", mReactance1);
	mSLog->info("Reactance={} [Ohm] (secondary side)", mReactance2);
	mSLog->info("Reactance={} [Ohm] (tetrary side)", mReactance3);


	// Component parameters are referred to high voltage side.
	// Switch terminals if transformer is connected the other way around.
	// Without modifying the mNominalVoltageEnd value.
	if (mNominalVoltageEnd1 < mNominalVoltageEnd2 && 
		mNominalVoltageEnd1 < mNominalVoltageEnd3) {
		std::shared_ptr<SimTerminal<Complex>> tmp = mTerminals[0];
		if (mNominalVoltageEnd2 > mNominalVoltageEnd3) {
			mTerminals[0] = mTerminals[1];
			mTerminals[1] = mTerminals[2];
		}
		else {
			mTerminals[0] = mTerminals[2];
		}
		mTerminals[2] = tmp;
	}
	else if (mNominalVoltageEnd1 < mNominalVoltageEnd2) {
		std::shared_ptr<SimTerminal<Complex>> tmp = mTerminals[0];
		mTerminals[0] = mTerminals[1];
		mTerminals[1] = tmp;
	}
	else if (mNominalVoltageEnd1 < mNominalVoltageEnd3) {
		std::shared_ptr<SimTerminal<Complex>> tmp = mTerminals[0];
		mTerminals[0] = mTerminals[2];
		mTerminals[2] = mTerminals[1];
		mTerminals[1] = tmp;
	}
	else if (mNominalVoltageEnd2 < mNominalVoltageEnd3) {
		std::shared_ptr<SimTerminal<Complex>> tmp = mTerminals[1];
		mTerminals[1] = mTerminals[2];
		mTerminals[2] = tmp;
	}
	
	// TODO: Temporary for some reason its resetting to 1x1 the matrix ignoring L22 and L23
	mIntfVoltage = MatrixComp::Zero(3, 1);
	mIntfCurrent = MatrixComp::Zero(3, 1);

	// Static calculations from load flow data
	Complex impedance1 = { mResistance1, mReactance1 };
	Complex impedance2 = { mResistance2, mReactance2 };
	Complex impedance3 = { mResistance3, mReactance3 };

	// Set initial voltage of virtual node in between
	mVirtualNodes[1]->setInitialVoltage(initialSingleVoltage(0));
	mVirtualNodes[2]->setInitialVoltage(initialSingleVoltage(1));
	mVirtualNodes[3]->setInitialVoltage(initialSingleVoltage(2));

	mIntfVoltage(0, 0) = mVirtualNodes[0]->initialSingleVoltage() - initialSingleVoltage(0) * mRatio1;
	mIntfVoltage(1, 0) = mVirtualNodes[0]->initialSingleVoltage() - initialSingleVoltage(1) * mRatio2;
	mIntfVoltage(2, 0) = mVirtualNodes[0]->initialSingleVoltage() - initialSingleVoltage(2) * mRatio3;
	
	mIntfCurrent(0, 0) = mIntfVoltage(0, 0) / impedance1;
	mIntfCurrent(1, 0) = mIntfVoltage(1, 0) / impedance2;
	mIntfCurrent(2, 0) = mIntfVoltage(2, 0) / impedance3;

	// Create series sub components
	mSubInductor1 = std::make_shared<SP::Ph1::Inductor>(mUID + "_ind1", mName + "_ind1", Logger::Level::off);
	mSubInductor1->setParameters(mInductance1);
	mSubInductor2 = std::make_shared<SP::Ph1::Inductor>(mUID + "_ind2", mName + "_ind2", Logger::Level::off);
	mSubInductor2->setParameters(mInductance2);
	mSubInductor3 = std::make_shared<SP::Ph1::Inductor>(mUID + "_ind3", mName + "_ind3", Logger::Level::off);
	mSubInductor3->setParameters(mInductance3);


	if (mNumVirtualNodes == 7) {
		mVirtualNodes[4]->setInitialVoltage(initialSingleVoltage(0));
		mSubResistor1 = std::make_shared<SP::Ph1::Resistor>(mUID + "_res1", mName + "_res1", Logger::Level::off);
		mSubResistor1->setParameters(mResistance1);
		mSubResistor1->connect({ node(0), mVirtualNodes[4] });
		mSubResistor1->initialize(mFrequencies);
		mSubResistor1->initializeFromNodesAndTerminals(frequency);
		mSubInductor1->connect({ mVirtualNodes[4], mVirtualNodes[1] });

		mVirtualNodes[5]->setInitialVoltage(initialSingleVoltage(1));
		mSubResistor2 = std::make_shared<SP::Ph1::Resistor>(mUID + "_res2", mName + "_res2", Logger::Level::off);
		mSubResistor2->setParameters(mResistance1);
		mSubResistor2->connect({ node(1), mVirtualNodes[5] });
		mSubResistor2->initialize(mFrequencies);
		mSubResistor2->initializeFromNodesAndTerminals(frequency);
		mSubInductor2->connect({ mVirtualNodes[5], mVirtualNodes[2] });

		mVirtualNodes[6]->setInitialVoltage(initialSingleVoltage(2));
		mSubResistor2 = std::make_shared<SP::Ph1::Resistor>(mUID + "_res3", mName + "_res3", Logger::Level::off);
		mSubResistor2->setParameters(mResistance1);
		mSubResistor2->connect({ node(2), mVirtualNodes[6] });
		mSubResistor2->initialize(mFrequencies);
		mSubResistor2->initializeFromNodesAndTerminals(frequency);
		mSubInductor2->connect({ mVirtualNodes[6], mVirtualNodes[3] });

	} else {
		mSubInductor1->connect({ node(0), mVirtualNodes[1] });
		mSubInductor2->connect({ node(1), mVirtualNodes[2] });
		mSubInductor3->connect({ node(2), mVirtualNodes[3] });
	}
	mSubInductor1->initialize(mFrequencies);
	mSubInductor1->initializeFromNodesAndTerminals(frequency);
	mSubInductor2->initialize(mFrequencies);
	mSubInductor2->initializeFromNodesAndTerminals(frequency);
	mSubInductor3->initialize(mFrequencies);
	mSubInductor3->initializeFromNodesAndTerminals(frequency);

	// Create parallel sub components
	// A snubber conductance is added on the low voltage side (resistance scaled with lower voltage side)
	mSnubberResistance = mNominalVoltageEnd1 > mNominalVoltageEnd2 ? std::abs(mNominalVoltageEnd2)*1e6 : std::abs(mNominalVoltageEnd1)*1e6;
	mSubSnubResistor = std::make_shared<SP::Ph1::Resistor>(mUID + "_snub_res", mName + "_snub_res", Logger::Level::off);
	mSubSnubResistor->setParameters(mSnubberResistance);
	mSubSnubResistor->connect({ node(2), SP::SimNode::GND });
	mSubSnubResistor->initialize(mFrequencies);
	mSubSnubResistor->initializeFromNodesAndTerminals(frequency);
	mSLog->info("Snubber Resistance={} [Ohm] (connected to LV side)", mSnubberResistance);

	mSLog->info(
		"\n--- Initialization from powerflow ---"
		"\nVoltage across 1: {:s}"
		"\nCurrent 1: {:s}"
		"\nVoltage across 2: {:s}"
		"\nCurrent 2: {:s}"
		"\nVoltage across 3: {:s}"
		"\nCurrent 3: {:s}"
		"\nTerminal 1 voltage: {:s}"
		"\nTerminal 2 voltage: {:s}"
		"\nTerminal 3 voltage: {:s}"
		"\n--- Initialization from powerflow finished ---",
		Logger::phasorToString(mIntfVoltage(0, 0)),
		Logger::phasorToString(mIntfCurrent(0, 0)),
		Logger::phasorToString(mIntfVoltage(1, 0)),
		Logger::phasorToString(mIntfCurrent(1, 0)),
		Logger::phasorToString(mIntfVoltage(2, 0)),
		Logger::phasorToString(mIntfCurrent(2, 0)),
		Logger::phasorToString(initialSingleVoltage(0)),
		Logger::phasorToString(initialSingleVoltage(1)),
		Logger::phasorToString(initialSingleVoltage(2)));
}


// #### Powerflow section ####

void SP::Ph1::Transformer3W::setBaseVoltage(Real baseVoltage) {
	// Note: to be consistent set base voltage to higher voltage (and impedance values must be referred to high voltage side)
	// TODO: use attribute setter for setting base voltage
    mBaseVoltage = baseVoltage;
}

void SP::Ph1::Transformer3W::calculatePerUnitParameters(Real baseApparentPower, Real baseOmega) {
	mSLog->info("#### Calculate Per Unit Parameters for {}", mName);
    mBaseApparentPower = baseApparentPower;
	mBaseOmega = baseOmega;
    mSLog->info("Base Power={} [VA]  Base Omega={} [1/s]", baseApparentPower, baseOmega);

	mBaseImpedance = mBaseVoltage * mBaseVoltage / mBaseApparentPower;
	mBaseAdmittance = 1.0 / mBaseImpedance;
	mBaseCurrent = baseApparentPower / (mBaseVoltage*sqrt(3)); // I_base=(S_threephase/3)/(V_line_to_line/sqrt(3))
	mSLog->info("Base Voltage={} [V]  Base Impedance={} [Ohm]", mBaseVoltage, mBaseImpedance);

	mResistancePerUnit1 = mResistance1 / mBaseImpedance;
	mReactancePerUnit1 = mReactance1 / mBaseImpedance;
    mSLog->info("Resistance={} [pu]  Reactance={} [pu]", mResistancePerUnit1, mReactancePerUnit1);

	mResistancePerUnit2 = mResistance2 / mBaseImpedance;
	mReactancePerUnit2 = mReactance2 / mBaseImpedance;
    mSLog->info("Resistance={} [pu]  Reactance={} [pu]", mResistancePerUnit2, mReactancePerUnit2);

	mResistancePerUnit3 = mResistance3 / mBaseImpedance;
	mReactancePerUnit3 = mReactance3 / mBaseImpedance;
    mSLog->info("Resistance={} [pu]  Reactance={} [pu]", mResistancePerUnit3, mReactancePerUnit3);

	mBaseInductance = mBaseImpedance / mBaseOmega;
	mInductancePerUnit1 = mInductance1 / mBaseInductance;
	mInductancePerUnit2 = mInductance2 / mBaseInductance;
	mInductancePerUnit3 = mInductance3 / mBaseInductance;
	mMagnetizing = Complex(0., mMagnetizingReactance);
	// omega per unit=1, hence 1.0*mInductancePerUnit.
	mLeakagePerUnit1 = Complex(mResistancePerUnit1, 1.*mInductancePerUnit1);
	mLeakagePerUnit2 = Complex(mResistancePerUnit2, 1.*mInductancePerUnit2);
	mLeakagePerUnit3 = Complex(mResistancePerUnit3, 1.*mInductancePerUnit3);
	mMagnetizingPerUnit = mMagnetizing / mBaseImpedance;

	// set snubber resistance
	if (mBehaviour == Behaviour::Initialization) {
		Real snubberResistance = 1e3;
		mSubSnubResistor = std::make_shared<SP::Ph1::Resistor>(mUID + "_snub_res", mName + "_snub_res", mLogLevel);
		mSubSnubResistor->setParameters(snubberResistance);
		mSubSnubResistor->connect({ node(2), SP::SimNode::GND });
		mSubSnubResistor->initializeFromNodesAndTerminals(mBaseOmega);
	}
	if (mSubSnubResistor) {
		mSubSnubResistor->setBaseVoltage(mBaseVoltage);
		mSubSnubResistor->calculatePerUnitParameters(baseApparentPower);
	}

	mSLog->info("Leakage Impedance 12 Per Unit={} [pu] ", mLeakagePerUnit1 + mLeakagePerUnit2);
	mSLog->info("Leakage Impedance 23 Per Unit={} [pu] ", mLeakagePerUnit2 + mLeakagePerUnit3);
	mSLog->info("Leakage Impedance 31 Per Unit={} [pu] ", mLeakagePerUnit3 + mLeakagePerUnit1);
}

void SP::Ph1::Transformer3W::pfApplyAdmittanceMatrixStamp(SparseMatrixCompRow & Y) {
	// calculate matrix stamp
	mY_element = MatrixComp(3, 3);
	Complex a, b, c, d;

	a = mLeakagePerUnit2 * mLeakagePerUnit3 * mRatioAbs1;
	b = mLeakagePerUnit3 * mLeakagePerUnit1 * mRatioAbs2;
	c = mLeakagePerUnit1 * mLeakagePerUnit2 * mRatioAbs3;
	
	if ((std::abs(mMagnetizingPerUnit) > 1e9)) {
		d = a * mRatioAbs1 + b * mRatioAbs2 + c * mRatioAbs3;
	}
	else {
		d = a * mRatioAbs1 + b * mRatioAbs2 + c * mRatioAbs3 + 
				(mLeakagePerUnit1 * mLeakagePerUnit2 * mLeakagePerUnit3 / mMagnetizingPerUnit);
	}

	mY_element(0, 0) = (d - mRatioAbs1 * a) / (mLeakagePerUnit1 * d);
	mY_element(0, 1) = - mRatioAbs1 * b / (mLeakagePerUnit1 * d);
	mY_element(0, 2) = - mRatioAbs1 * c / (mLeakagePerUnit1 * d);

	mY_element(1, 0) = - mRatioAbs2 * a / (mLeakagePerUnit2 * d);
	mY_element(1, 1) = (d - mRatioAbs2 * b) / (mLeakagePerUnit2 * d);
	mY_element(1, 2) = - mRatioAbs2 * c / (mLeakagePerUnit2 * d);
	
	mY_element(2, 0) = - mRatioAbs3 * a / (mLeakagePerUnit3 * d);
	mY_element(2, 1) = - mRatioAbs3 * b / (mLeakagePerUnit3 * d);
	mY_element(2, 2) = (d - mRatioAbs3 * c) / (mLeakagePerUnit3 * d);

	//check for inf or nan
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			if (std::isinf(mY_element.coeff(i, j).real()) || std::isinf(mY_element.coeff(i, j).imag())) {
				std::cout << mY_element << std::endl;
				std::cout << "Zm:" << mMagnetizing << std::endl;
				std::stringstream ss;
				ss << "Transformer>>" << this->name() << ": infinite or nan values in the element Y at: " << i << "," << j;
				throw std::invalid_argument(ss.str());
			}

	//set the circuit matrix values
	Y.coeffRef(this->matrixNodeIndex(0), this->matrixNodeIndex(0)) += mY_element.coeff(0, 0);
	Y.coeffRef(this->matrixNodeIndex(0), this->matrixNodeIndex(1)) += mY_element.coeff(0, 1);
	Y.coeffRef(this->matrixNodeIndex(0), this->matrixNodeIndex(2)) += mY_element.coeff(0, 2);

	Y.coeffRef(this->matrixNodeIndex(1), this->matrixNodeIndex(0)) += mY_element.coeff(1, 0);
	Y.coeffRef(this->matrixNodeIndex(1), this->matrixNodeIndex(1)) += mY_element.coeff(1, 1);
	Y.coeffRef(this->matrixNodeIndex(1), this->matrixNodeIndex(2)) += mY_element.coeff(1, 2);

	Y.coeffRef(this->matrixNodeIndex(2), this->matrixNodeIndex(0)) += mY_element.coeff(2, 0);
	Y.coeffRef(this->matrixNodeIndex(2), this->matrixNodeIndex(1)) += mY_element.coeff(2, 1);
	Y.coeffRef(this->matrixNodeIndex(2), this->matrixNodeIndex(2)) += mY_element.coeff(2, 2);

	mSLog->info("#### Y matrix stamping: {}", mY_element);
}

void SP::Ph1::Transformer3W::updateBranchFlow(VectorComp& current, VectorComp& powerflow) {
	mCurrent = current * mBaseCurrent;
	mActivePowerBranch = powerflow.real()*mBaseApparentPower;
	mReactivePowerBranch = powerflow.imag()*mBaseApparentPower;
}

void SP::Ph1::Transformer3W::storeNodalInjection(Complex powerInjection) {
	mActivePowerInjection = std::real(powerInjection)*mBaseApparentPower;
	mReactivePowerInjection = std::imag(powerInjection)*mBaseApparentPower;
	mStoreNodalPowerInjection = true;
}

MatrixComp SP::Ph1::Transformer3W::Y_element() {
	return mY_element;
}