#include "SynchronGeneratorEMT.h"

using namespace DPsim;

SynchronGeneratorEMT::SynchronGeneratorEMT(std::string name, int node1, int node2, int node3,
	SynchGenStateType stateType, Real nomPower, Real nomVolt, Real nomFreq, int poleNumber, Real nomFieldCur,
	SynchGenParamType paramType, Real Rs, Real Ll, Real Lmd, Real Lmd0, Real Lmq, Real Lmq0,
	Real Rfd, Real Llfd, Real Rkd, Real Llkd,
	Real Rkq1, Real Llkq1, Real Rkq2, Real Llkq2,
	Real inertia) {

	this->mNode1 = node1 - 1;
	this->mNode2 = node2 - 1;
	this->mNode3 = node3 - 1;

	mStateType = stateType;
	mNomPower = nomPower;
	mNomVolt = nomVolt;
	mNomFreq = nomFreq;
	mPoleNumber = poleNumber;
	mNomFieldCur = nomFieldCur;

	// base stator values
	mBase_V_RMS = mNomVolt / sqrt(3);
	mBase_v = mBase_V_RMS * sqrt(2);
	mBase_I_RMS = mNomPower / (3 * mBase_V_RMS);
	mBase_i = mBase_I_RMS * sqrt(2);
	mBase_Z = mBase_v / mBase_i;
	mBase_OmElec = 2 * DPS_PI * mNomFreq;
	mBase_OmMech = mBase_OmElec / (mPoleNumber / 2);
	mBase_L = mBase_Z / mBase_OmElec;
	mBase_Psi = mBase_L * mBase_i;
	mBase_T = mNomPower / mBase_OmMech;

	if (paramType == SynchGenParamType::perUnit) {
		// steady state per unit initial value
		initWithPerUnitParam(Rs, Ll, Lmd, Lmd0, Lmq, Lmq0, Rfd, Llfd, Rkd, Llkd, Rkq1, Llkq1, Rkq2, Llkq2, inertia);
	}
	else	if (paramType == SynchGenParamType::statorReferred) {
		// steady state stator referred initial value
		initWithStatorRefParam(Rs, Ll, Lmd, Lmd0, Lmq, Lmq0, Rfd, Llfd, Rkd, Llkd, Rkq1, Llkq1, Rkq2, Llkq2, inertia);
	}
}

void SynchronGeneratorEMT::initWithPerUnitParam(
	Real Rs, Real Ll, Real Lmd, Real Lmd0, Real Lmq, Real Lmq0,
	Real Rfd, Real Llfd, Real Rkd, Real Llkd,
	Real Rkq1, Real Llkq1, Real Rkq2, Real Llkq2,
	Real H) {

	// base rotor values
	mBase_ifd = Lmd * mNomFieldCur;
	mBase_vfd = mNomPower / mBase_ifd;
	mBase_Zfd = mBase_vfd / mBase_ifd;
	mBase_Lfd = mBase_Zfd / mBase_OmElec;

	if (mStateType == SynchGenStateType::perUnit) {
		mRs = Rs;
		mLl = Ll;
		mLmd = Lmd;
		mLmd0 = Lmd0;
		mLmq = Lmq;
		mLmq0 = Lmq0;
		mRfd = Rfd;
		mLlfd = Llfd;
		mRkd = Rkd;
		mLlkd = Llkd;
		mRkq1 = Rkq1;
		mLlkq1 = Llkq1;
		mRkq2 = Rkq2;
		mLlkq2 = Llkq2;
		mH = H;
		// Additional inductances according to Krause
		mLaq = 1 / (1 / mLmq + 1 / mLl + 1 / mLlkq1 + 1 / mLlkq2);
		mLad = 1 / (1 / mLmd + 1 / mLl + 1 / mLlkd + 1 / mLlfd);
	}
	else if (mStateType == SynchGenStateType::statorReferred) {
		mRs = Rs * mBase_Z;
		mLl = Ll * mBase_L;
		mLmd = Lmd * mBase_L;
		mLmd0 = Lmd0 * mBase_L;
		mLmq = Lmq * mBase_L;
		mLmq0 = Lmq0 * mBase_L;
		mRfd = Rfd * mBase_Z;
		mLlfd = Llfd * mBase_L;
		mRkd = Rkd * mBase_Z;
		mLlkd = Llkd * mBase_L;
		mRkq1 = Rkq1 * mBase_Z;
		mLlkq1 = Llkq1 * mBase_L;
		mRkq2 = Rkq2 * mBase_Z;
		mLlkq2 = Llkq2 * mBase_L;
		// Additional inductances according to Krause
		mLaq = 1 / (1 / mLmq + 1 / mLl + 1 / mLlkq1 + 1 / mLlkq2) * mBase_L;
		mLad = 1 / (1 / mLmd + 1 / mLl + 1 / mLlkd + 1 / mLlfd) * mBase_L;
	}
}

void SynchronGeneratorEMT::initWithStatorRefParam(
	Real Rs, Real Ll, Real Lmd, Real Lmd0, Real Lmq, Real Lmq0,
	Real Rfd, Real Llfd, Real Rkd, Real Llkd,
	Real Rkq1, Real Llkq1, Real Rkq2, Real Llkq2,
	Real J) {

	// base rotor values
	mBase_ifd = Lmd / mBase_L * mNomFieldCur;
	mBase_vfd = mNomPower / mBase_ifd;
	mBase_Zfd = mBase_vfd / mBase_ifd;
	mBase_Lfd = mBase_Zfd / mBase_OmElec;
	
	if (mStateType == SynchGenStateType::perUnit) {
		// combination not supported
	}
	else if (mStateType == SynchGenStateType::statorReferred) {
		mRs = Rs;
		mLl = Ll;
		mLmd = Lmd;
		mLmd0 = Lmd0;
		mLmq = Lmq;
		mLmq0 = Lmq0;
		mRfd = Rfd;
		mLlfd = Llfd;
		mRkd = Rkd;
		mLlkd = Llkd;
		mRkq1 = Rkq1;
		mLlkq1 = Llkq1;
		mRkq2 = Rkq2;
		mLlkq2 = Llkq2;
		mJ = J;
		mOmMech = mBase_OmMech;
		// Additional inductances according to Krause
		mLaq = 1 / (1 / mLmq + 1 / mLl + 1 / mLlkq1 + 1 / mLlkq2);
		mLad = 1 / (1 / mLmd + 1 / mLl + 1 / mLlkd + 1 / mLlfd);
	}
}

void SynchronGeneratorEMT::init(Real om, Real dt,
	Real initActivePower, Real initReactivePower, Real initTerminalVolt, Real initVoltAngle) {

	// Create matrices for state space representation 
	mInductanceMat << 
		mLl + mLmq, 0, 0, mLmq, mLmq, 0, 0,
		0, mLl + mLmd, 0, 0, 0, mLmd, mLmd,
		0, 0, mLl, 0, 0, 0, 0,
		mLmq, 0, 0, mLlkq1 + mLmq, mLmq, 0, 0,
		mLmq, 0, 0, mLmq, mLlkq2 + mLmq, 0, 0,
		0, mLmd, 0, 0, 0, mLlfd + mLmd, mLmd,
		0, mLmd, 0, 0, 0, mLmd, mLlkd + mLmd;

	mResistanceMat << 
		mRs, 0, 0, 0, 0, 0, 0,
		0, mRs, 0, 0, 0, 0, 0,
		0, 0, mRs, 0, 0, 0, 0,
		0, 0, 0, mRkq1, 0, 0, 0,
		0, 0, 0, 0, mRkq2, 0, 0,
		0, 0, 0, 0, 0, mRfd, 0,
		0, 0, 0, 0, 0, 0, mRkd;

	mOmegaFluxMat << 
		0, 1, 0, 0, 0, 0, 0,
		-1, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0;

	mReverseCurrents <<	
		-1, 0, 0, 0, 0, 0, 0,
		0, -1, 0, 0, 0, 0, 0,
		0, 0, -1, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 1;

	mReactanceMat = mInductanceMat.inverse();

	if (mStateType == SynchGenStateType::perUnit) {
		// steady state per unit initial value
		initStatesInPerUnit(initActivePower, initReactivePower, initTerminalVolt, initVoltAngle);
	} else	if (mStateType == SynchGenStateType::statorReferred) {
		// steady state stator referred initial value
		initStatesInStatorRefFrame(initActivePower, initReactivePower, initTerminalVolt, initVoltAngle);		
	}	

	mDq0Voltages(0, 0) = mVoltages(0, 0);
	mDq0Voltages(1, 0) = mVoltages(1, 0);
	mDq0Voltages(2, 0) = mVoltages(2, 0);	
	mDq0Voltages = mDq0Voltages * mBase_v;
	mAbcsVoltages = inverseParkTransform(mThetaMech, mDq0Voltages);

	mDq0Currents(0, 0) = mCurrents(0, 0);
	mDq0Currents(1, 0) = mCurrents(1, 0);
	mDq0Currents(2, 0) = mCurrents(2, 0);
	mDq0Currents = mDq0Currents * mBase_i;
	mAbcsCurrents = inverseParkTransform(mThetaMech, mDq0Currents);
}

void SynchronGeneratorEMT::initStatesInPerUnit(Real initActivePower, Real initReactivePower,
	Real initTerminalVolt, Real initVoltAngle) {

	double init_P = initActivePower / mNomPower;
	double init_Q = initReactivePower / mNomPower;
	double init_S = sqrt(pow(init_P, 2.) + pow(init_Q, 2.));
	double init_vt = initTerminalVolt / mBase_v;
	double init_it = init_S / init_vt;

	// power factor
	double init_pf = acos(init_P / init_S);

	// load angle
	double init_delta = atan(((mLmq + mLl) * init_it * cos(init_pf) - mRs * init_it * sin(init_pf)) /
		(init_vt + mRs * init_it * cos(init_pf) + (mLmq + mLl) * init_it * sin(init_pf)));
	double init_delta_deg = init_delta / DPS_PI * 180;

	// dq stator voltages and currents
	double init_vd = init_vt * sin(init_delta);
	double init_vq = init_vt * cos(init_delta);
	double init_id = init_it * sin(init_delta + init_pf);
	double init_iq = init_it * cos(init_delta + init_pf);

	// rotor voltage and current
	double init_ifd = (init_vq + mRs * init_iq + (mLmd + mLl) * init_id) / mLmd;
	double init_vfd = mRfd * init_ifd;

	// flux linkages
	double init_psid = init_vq + mRs * init_iq;
	double init_psiq = -init_vd - mRs * init_id;
	double init_psifd = (mLmd + mLlfd) * init_ifd - mLmd * init_id;
	double init_psid1 = mLmd * (init_ifd - init_id);
	double init_psiq1 = -mLmq * init_iq;
	double init_psiq2 = -mLmq * init_iq;

	// rotor mechanical variables
	double init_Te = init_P + mRs * pow(init_it, 2.);
	mOmMech = 1;

	mVoltages(0, 0) = init_vq;
	mVoltages(1, 0) = init_vd;
	mVoltages(2, 0) = 0;
	mVoltages(3, 0) = 0;
	mVoltages(4, 0) = 0;
	mVoltages(5, 0) = init_vfd;
	mVoltages(6, 0) = 0;

	mCurrents(0, 0) = init_iq;
	mCurrents(1, 0) = init_id;
	mCurrents(2, 0) = 0;
	mCurrents(3, 0) = 0;
	mCurrents(4, 0) = 0;
	mCurrents(5, 0) = init_ifd;
	mCurrents(6, 0) = 0;

	mFluxes(0, 0) = init_psiq;
	mFluxes(1, 0) = init_psid;
	mFluxes(2, 0) = 0;
	mFluxes(3, 0) = init_psiq1;
	mFluxes(4, 0) = init_psiq2;
	mFluxes(5, 0) = init_psifd;
	mFluxes(6, 0) = init_psid1;

	// Initialize mechanical angle
	mThetaMech = initVoltAngle + init_delta;
}

void SynchronGeneratorEMT::initStatesInStatorRefFrame(Real initActivePower, Real initReactivePower,
	Real initTerminalVolt, Real initVoltAngle) {

	double init_P = initActivePower;
	double init_Q = initReactivePower;
	double init_S = sqrt(pow(init_P, 2.) + pow(init_Q, 2.));
	double init_vt = initTerminalVolt / mBase_v;
	double init_it = init_S / init_vt;

	// power factor
	double init_pf = acos(init_P / init_S);

	// load angle
	double init_delta = atan(((mLmq + mLl) * mBase_OmElec * init_it * cos(init_pf) - mRs * init_it * sin(init_pf)) /
		(init_vt + mRs * init_it * cos(init_pf) + (mLmq + mLl) * mBase_OmElec * init_it * sin(init_pf)));
	double init_delta_deg = init_delta / DPS_PI * 180;

	// dq stator voltages and currents
	double init_vd = init_vt * sin(init_delta);
	double init_vq = init_vt * cos(init_delta);
	double init_id = init_it * sin(init_delta + init_pf);
	double init_iq = init_it * cos(init_delta + init_pf);

	// rotor voltage and current
	double init_ifd = (init_vq + mRs * init_iq + (mLmd + mLl) * init_id) / mLmd;
	// TODO calculate remaining initial values

	// rotor mechanical variables, TODO torque
	mOmMech = mBase_OmMech;

	mCurrents(0, 0) = init_iq;
	mCurrents(1, 0) = init_id;
	mCurrents(2, 0) = 0;
	mCurrents(3, 0) = 0;
	mCurrents(4, 0) = 0;
	mCurrents(5, 0) = init_ifd;
	mCurrents(6, 0) = 0;

	mVoltages(0, 0) = init_vq;
	mVoltages(1, 0) = init_vd;
	mVoltages(2, 0) = 0;

	// Initialize mechanical angle
	mThetaMech = initVoltAngle + init_delta;
}

void SynchronGeneratorEMT::step(SystemModel& system, Real fieldVoltage, Real mechPower) {

	if (mStateType == SynchGenStateType::perUnit) {
		stepInPerUnit(system.getOmega(), system.getTimeStep(), fieldVoltage, mechPower);
	}
	else if (mStateType == SynchGenStateType::statorReferred) {
		stepInStatorRefFrame(system.getOmega(), system.getTimeStep(), fieldVoltage, mechPower);
	}

	// Update current source accordingly
	if (mNode1 >= 0) {
		system.addRealToRightSideVector(mNode1, mAbcsCurrents(0, 0));
	}
	if (mNode2 >= 0) {
		system.addRealToRightSideVector(mNode2, mAbcsCurrents(1, 0));
	}
	if (mNode3 >= 0) {
		system.addRealToRightSideVector(mNode3, mAbcsCurrents(2, 0));
	}
}

void SynchronGeneratorEMT::stepInPerUnit(Real om, Real dt, Real fieldVoltage, Real mechPower) {
	// retrieve voltages
	mAbcsVoltages = (1 / mBase_v) * mAbcsVoltages;
	mAbcsCurrents = (1 / mBase_i) * mAbcsCurrents;
	// mVoltages(5, 0) = fieldVoltage / mBase_v;
	// TODO calculate effect of changed field voltage

	// dq-transform of interface voltage
	mDq0Voltages = parkTransform(mThetaMech, mAbcsVoltages);
	mVoltages(0, 0) = mDq0Voltages(0, 0);
	mVoltages(1, 0) = mDq0Voltages(1, 0);
	mVoltages(2, 0) = mDq0Voltages(2, 0);

	// calculate mechanical states
	mMechPower = mechPower / mNomPower;
	mMechTorque = mMechPower / mOmMech;
	mElecTorque = (mFluxes(1, 0)*mCurrents(0, 0) - mFluxes(0, 0)*mCurrents(1, 0));

	// Euler step forward	
	mOmMech = mOmMech + dt * (1 / (2 * mH) * (mMechTorque - mElecTorque));
	DPSMatrix currents = mReverseCurrents * mReactanceMat * mFluxes;
	DPSMatrix dtFluxes = mVoltages - mResistanceMat * currents - mOmMech * mOmegaFluxMat * mFluxes;
	for (int i = 0; i < dtFluxes.size(); i++)
	{
		if (dtFluxes(i, 0) < 0.000001)
			dtFluxes(i, 0) = 0;
	}
	mFluxes = mFluxes + dt * mBase_OmElec * dtFluxes;
	
	mCurrents = mReverseCurrents * mReactanceMat * mFluxes;

	// Update mechanical rotor angle with respect to electrical angle
	mThetaMech = mThetaMech + dt * (mOmMech * mBase_OmMech);

	// inverse dq-transform
	mDq0Currents(0, 0) = mCurrents(0, 0);
	mDq0Currents(1, 0) = mCurrents(1, 0);
	mDq0Currents(2, 0) = mCurrents(2, 0);
	mAbcsCurrents = inverseParkTransform(mThetaMech, mDq0Currents);
	mAbcsCurrents = mBase_i * mAbcsCurrents;

	
}

void SynchronGeneratorEMT::stepInStatorRefFrame(Real om, Real dt, Real fieldVoltage, Real mechPower) {
	// retrieve voltages 
	mVoltages(5, 0) = fieldVoltage;
	// TODO mCurrents(5, 0) = fieldCurrent;

	// dq-transform of interface voltage
	mDq0Voltages = parkTransform(mThetaMech, mAbcsVoltages);
	mVoltages(0, 0) = mDq0Voltages(0, 0);
	mVoltages(1, 0) = mDq0Voltages(1, 0);
	mVoltages(2, 0) = mDq0Voltages(2, 0);

	// calculate mechanical states
	mMechPower = mechPower;
	mMechTorque = mechPower / mOmMech;

	// Euler step forward
	DPSMatrix test1 = mVoltages;
	DPSMatrix test2 = mOmMech * mOmegaFluxMat * mFluxes;
	DPSMatrix test3 = mResistanceMat * mCurrents;//mReactanceMat * mFluxes;

	//mFluxes = mFluxes + dt * (mVoltages - mResistanceMat * mReactanceMat * mFluxes - mOmega_r * mOmegaFluxMat * mFluxes);

	// inverse dq-transform
	mCurrents = mReactanceMat * mFluxes;
	mDq0Currents(0, 0) = -mCurrents(0, 0);
	mDq0Currents(1, 0) = -mCurrents(1, 0);
	mDq0Currents(2, 0) = -mCurrents(2, 0);
	mAbcsCurrents = inverseParkTransform(mThetaMech, mDq0Currents);

	//mTheta_r = mTheta_r + dt * mOmega_r;
	mElecTorque = -3. * mPoleNumber / 4. * (mFluxes(1, 0)*mCurrents(0, 0) - mFluxes(0, 0)*mCurrents(1, 0));
	//mOmega_r = mOmega_r + dt * (mPoleNumber / (2 * mInertia) * (mMechTorque - mElecTorque));
}

void SynchronGeneratorEMT::postStep(SystemModel& system) {
	if (mNode1 >= 0) {
		mAbcsVoltages(0,0) = system.getRealFromLeftSideVector(mNode1);		
	}
	else {
		mAbcsVoltages(0, 0) = 0;		
	}
	if (mNode2 >= 0) {
		mAbcsVoltages(1, 0) = system.getRealFromLeftSideVector(mNode2);
	}
	else {
		mAbcsVoltages(1, 0) = 0;		
	}
	if (mNode3 >= 0) {
		mAbcsVoltages(2, 0) = system.getRealFromLeftSideVector(mNode3);
	}
	else {
		mAbcsVoltages(2, 0) = 0;
	}
}

DPSMatrix SynchronGeneratorEMT::parkTransform(Real theta, DPSMatrix& in) {
	DPSMatrix ParkMat(3,3);
	ParkMat << 
		2. / 3. * cos(theta), 2. / 3. * cos(theta - 2. * M_PI / 3.), 2. / 3. * cos(theta + 2. * M_PI / 3.),
		2. / 3. * sin(theta), 2. / 3. * sin(theta - 2. * M_PI / 3.), 2. / 3. * sin(theta + 2. * M_PI / 3.),
		1. / 3., 1. / 3., 1. / 3.;

	// Park transform according to Kundur
	// ParkMat << 2. / 3. * cos(theta), 2. / 3. * cos(theta - 2. * M_PI / 3.), 2. / 3. * cos(theta + 2. * M_PI / 3.),
	//	- 2. / 3. * sin(theta), - 2. / 3. * sin(theta - 2. * M_PI / 3.), - 2. / 3. * sin(theta + 2. * M_PI / 3.),
	//	1. / 3., 1. / 3., 1. / 3.;

	return ParkMat * in;
}

DPSMatrix SynchronGeneratorEMT::inverseParkTransform(Real theta, DPSMatrix& in) {
	DPSMatrix InverseParkMat(3,3);
	InverseParkMat << 
		cos(theta), sin(theta), 1,
		cos(theta - 2. * M_PI / 3.), sin(theta - 2. * M_PI / 3.), 1,
		cos(theta + 2. * M_PI / 3.), sin(theta + 2. * M_PI / 3.), 1;

	return InverseParkMat * in;
}