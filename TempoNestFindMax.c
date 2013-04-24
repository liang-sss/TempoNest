//  Copyright (C) 2013 Lindley Lentati

/*
*    This file is part of TempoNest 
* 
*    TempoNest is free software: you can redistribute it and/or modify 
*    it under the terms of the GNU General Public License as published by 
*    the Free Software Foundation, either version 3 of the License, or 
*    (at your option) any later version. 
*    TempoNest  is distributed in the hope that it will be useful, 
*    but WITHOUT ANY WARRANTY; without even the implied warranty of 
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
*    GNU General Public License for more details. 
*    You should have received a copy of the GNU General Public License 
*    along with TempoNest.  If not, see <http://www.gnu.org/licenses/>. 
*/

/*
*    If you use TempoNest and as a byproduct both Tempo2 and MultiNest
*    then please acknowledge it by citing Lentati L., Alexander P., Hobson M. P. (2013) for TempoNest,
*    Hobbs, Edwards & Manchester (2006) MNRAS, Vol 369, Issue 2, 
*    pp. 655-672 (bibtex: 2006MNRAS.369..655H)
*    or Edwards, Hobbs & Manchester (2006) MNRAS, VOl 372, Issue 4,
*    pp. 1549-1574 (bibtex: 2006MNRAS.372.1549E) when discussing the
*    timing model and MultiNest Papers here.
*/


#include <stdio.h>
#include <vector>
#include "/usr/include/gsl/gsl_sf_gamma.h"
#include <gsl/gsl_multimin.h>
#include "dgemm.h"
#include "dgemv.h"
#include "dpotri.h"
#include "dpotrf.h"
#include "dpotrs.h"
#include "tempo2.h"
#include "TempoNest.h"


void vHRedLogLikeMax(double *Cube, int &ndim, int &npars, double &lnew, void *context)
{

	clock_t startClock,endClock;
	long double LDparams[ndim];
	double *EFAC;
	double EQUAD, redamp, redalpha;
	int pcount=0;



	for(int p=0;p< ((MNStruct *)context)->numFitTiming + ((MNStruct *)context)->numFitJumps; p++){
		LDparams[p]=Cube[p]*(((MNStruct *)context)->LDpriors[p][1]) + (((MNStruct *)context)->LDpriors[p][0]);
		//printf("CubeTM: %i %g \n", p, Cube[p]); 
	}


	for(int p=0;p<((MNStruct *)context)->numFitTiming;p++){
		((MNStruct *)context)->pulse->param[((MNStruct *)context)->TempoFitNums[p][0]].val[((MNStruct *)context)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)context)->numFitJumps;p++){
		((MNStruct *)context)->pulse->jumpVal[((MNStruct *)context)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	if(((MNStruct *)context)->numFitEFAC == 0){
		EFAC=new double[1];
		EFAC[0]=1;
// 		
	}
	else if(((MNStruct *)context)->numFitEFAC == 1){
		EFAC=new double[1];
		EFAC[0]=Cube[pcount];
		pcount++;
		
	}
	else if(((MNStruct *)context)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)context)->numFitEFAC];
		for(int p=0;p< ((MNStruct *)context)->numFitEFAC; p++){
			EFAC[p]=Cube[pcount];
			//printf("CubeEFAC: %i %g \n", pcount, Cube[pcount]); 
			pcount++;
		}
	}				

	if(((MNStruct *)context)->numFitEQUAD == 0){
		EQUAD=0;
// 		printf("EQUAD: %g \n",EQUAD);
	}
	else{
		
		EQUAD=pow(10.0,2*Cube[pcount]);
		pcount++;
		//printf("CubeEQUAD: %i %g \n", pcount, Cube[pcount]); 

	}


	redamp=Cube[pcount];
	pcount++;
	redalpha=Cube[pcount];
	pcount++;
  	

// 	startClock = clock();
	
	
	fastformBatsAll(((MNStruct *)context)->pulse,((MNStruct *)context)->numberpulsars);                /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)context)->pulse,((MNStruct *)context)->numberpulsars,1);       /* Form residuals */

	double secday=24*60*60;
	double LongestPeriod=1.0/pow(10.0,-5);
	double flo=1.0/LongestPeriod;

	double modelalpha=redalpha;
	double gwamp=pow(10.0,redamp);
	double gwampsquared=gwamp*gwamp*(pow((365.25*secday),2)/(12*M_PI*M_PI))*(pow(365.25,(1-modelalpha)))/(pow(flo,(modelalpha-1)));

	double timdiff=0;

	double covconst=gsl_sf_gamma(1-modelalpha)*sin(0.5*M_PI*modelalpha);
// 	printf("constants: %g %g \n",gwampsquared,covconst);


	
	double **CovMatrix = new double*[((MNStruct *)context)->pulse->nobs]; for(int o1=0;o1<((MNStruct *)context)->pulse->nobs;o1++)CovMatrix[o1]=new double[((MNStruct *)context)->pulse->nobs];

	for(int o1=0;o1<((MNStruct *)context)->pulse->nobs; o1++){

		for(int o2=0;o2<((MNStruct *)context)->pulse->nobs; o2++){
			timdiff=((MNStruct *)context)->pulse->obsn[o1].bat-((MNStruct *)context)->pulse->obsn[o2].bat;	
			double tau=2.0*M_PI*fabs(timdiff);
			double covsum=0;

			for(int k=0; k <=10; k++){
				covsum=covsum+pow(-1.0,k)*(pow(flo*tau,2*k))/(iter_factorial(2*k)*(2*k+1-modelalpha));

			}

			CovMatrix[o1][o2]=gwampsquared*(covconst*pow((flo*tau),(modelalpha-1)) - covsum);
// 			printf("%i %i %g %g %g\n",o1,o2,CovMatrix[o1][o2],fabs(timdiff),covsum);

			if(o1==o2){
				CovMatrix[o1][o2] += pow(((((MNStruct *)context)->pulse->obsn[o1].toaErr)*pow(10.0,-6))*EFAC[((MNStruct *)context)->sysFlags[o1]],2) + EQUAD;
			}

		}
	}

	double covdet=0;
	double *WorkRes = new double[((MNStruct *)context)->pulse->nobs];
	for(int o1=0;o1<((MNStruct *)context)->pulse->nobs; o1++){
		WorkRes[o1]=((MNStruct *)context)->pulse->obsn[o1].residual;
	}
	dpotrf(CovMatrix, ((MNStruct *)context)->pulse->nobs, covdet);
        dpotrs(CovMatrix, WorkRes, ((MNStruct *)context)->pulse->nobs);


	double Chisq=0;


	for(int o1=0;o1<((MNStruct *)context)->pulse->nobs; o1++){
		Chisq += ((MNStruct *)context)->pulse->obsn[o1].residual*WorkRes[o1];
	}

	if(isnan(covdet) || isinf(covdet) || isnan(Chisq) || isinf(Chisq)){

		lnew=-pow(10.0,200);
// 		printf("red amp and alpha %g %g\n",redamp,redalpha);
// 		printf("Like: %g %g %g \n",lnew,Chisq,covdet);
		
	}
	else{
		lnew = -0.5*(((MNStruct *)context)->pulse->nobs*log(2*M_PI) + covdet + Chisq);	
// 		printf("red amp and alpha %g %g\n",redamp,redalpha);


	}
// 	endClock = clock();
// //   	printf("Finishing off: time taken = %.2f (s)\n",(endClock-startClock)/(float)CLOCKS_PER_SEC);
	

	delete[] EFAC;
	for(int o=0;o<((MNStruct *)context)->pulse->nobs;o++){delete[] CovMatrix[o];}
	delete[] CovMatrix;
	delete[] WorkRes;
	//printf("Like: %g %g %g \n",lnew,Chisq,covdet);

}

void vHRedMarginLogLikeMax(double *Cube, int &ndim, int &npars, double &lnew, void *context)
{

	clock_t startClock,endClock;
	long double LDparams[ndim];
	double *EFAC;
	double EQUAD, redamp, redalpha;
	int pcount=0;


	for(int p=0;p< ((MNStruct *)context)->numFitTiming + ((MNStruct *)context)->numFitJumps; p++){
		LDparams[p]=Cube[p]*(((MNStruct *)context)->LDpriors[p][1]) + (((MNStruct *)context)->LDpriors[p][0]);
		//printf("CubeTM: %i %g \n", p, Cube[p]); 
	}


	for(int p=0;p<((MNStruct *)context)->numFitTiming;p++){
		((MNStruct *)context)->pulse->param[((MNStruct *)context)->TempoFitNums[p][0]].val[((MNStruct *)context)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)context)->numFitJumps;p++){
		((MNStruct *)context)->pulse->jumpVal[((MNStruct *)context)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	if(((MNStruct *)context)->numFitEFAC == 0){
		EFAC=new double[1];
		EFAC[0]=1;
// 		
	}
	else if(((MNStruct *)context)->numFitEFAC == 1){
		EFAC=new double[1];
		EFAC[0]=Cube[pcount];
		//printf("CubeEFAC: %i %g \n", pcount, Cube[pcount]); 
		pcount++;
		
	}
	else if(((MNStruct *)context)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)context)->numFitEFAC];
		for(int p=0;p< ((MNStruct *)context)->numFitEFAC; p++){
			EFAC[p]=Cube[pcount];
			//printf("CubeEFAC: %i %g \n", pcount, Cube[pcount]); 
			pcount++;
		}
	}				

	if(((MNStruct *)context)->numFitEQUAD == 0){
		EQUAD=0;
// 		printf("EQUAD: %g \n",EQUAD);
	}
	else{
		
		EQUAD=pow(10.0,2*Cube[pcount]);
		//printf("CubeEQUAD: %i %g \n", pcount, Cube[pcount]);
		pcount++;
		 

	}

	
	redamp=Cube[pcount];
	pcount++;
	redalpha=Cube[pcount];
	pcount++;
  	//printf("CubeRED: %g %g \n", Cube[ndim-2],Cube[ndim-1]);

// 	startClock = clock();
	
	
	fastformBatsAll(((MNStruct *)context)->pulse,((MNStruct *)context)->numberpulsars);                /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)context)->pulse,((MNStruct *)context)->numberpulsars,1);       /* Form residuals */

	double *WorkRes=new double[((MNStruct *)context)->pulse->nobs];
	double *GRes=new double[((MNStruct *)context)->Gsize];

	for(int o1=0;o1<((MNStruct *)context)->pulse->nobs; o1++){
		WorkRes[o1]=((MNStruct *)context)->pulse->obsn[o1].residual;
	}

	double secday=24*60*60;
	double LongestPeriod=1.0/pow(10.0,-5);
	double flo=1.0/LongestPeriod;

	double modelalpha=redalpha;
	double gwamp=pow(10.0,redamp);
	double gwampsquared=gwamp*gwamp*(pow((365.25*secday),2)/(12*M_PI*M_PI))*(pow(365.25,(1-modelalpha)))/(pow(flo,(modelalpha-1)));

	double timdiff=0;
	//printf("Get const\n");
	double covconst=gsl_sf_gamma(1-modelalpha)*sin(0.5*M_PI*modelalpha);
 	//printf("constants: %g %g \n",gwampsquared,covconst);


	
	double **CovMatrix = new double*[((MNStruct *)context)->pulse->nobs]; for(int o1=0;o1<((MNStruct *)context)->pulse->nobs;o1++)CovMatrix[o1]=new double[((MNStruct *)context)->pulse->nobs];

	for(int o1=0;o1<((MNStruct *)context)->pulse->nobs; o1++){

		for(int o2=0;o2<((MNStruct *)context)->pulse->nobs; o2++){
			timdiff=((MNStruct *)context)->pulse->obsn[o1].bat-((MNStruct *)context)->pulse->obsn[o2].bat;	
			double tau=2.0*M_PI*fabs(timdiff);
			double covsum=0;

			for(int k=0; k <=10; k++){
				covsum=covsum+pow(-1.0,k)*(pow(flo*tau,2*k))/(iter_factorial(2*k)*(2*k+1-modelalpha));

			}

			CovMatrix[o1][o2]=gwampsquared*(covconst*pow((flo*tau),(modelalpha-1)) - covsum);
// 			printf("%i %i %g %g %g\n",o1,o2,CovMatrix[o1][o2],fabs(timdiff),covsum);

			if(o1==o2){
				CovMatrix[o1][o2] += pow(((((MNStruct *)context)->pulse->obsn[o1].toaErr)*pow(10.0,-6))*EFAC[((MNStruct *)context)->sysFlags[o1]],2) + EQUAD;
			}

		}
	}

	double **CG = new double*[((MNStruct *)context)->pulse->nobs]; for(int o1=0;o1<((MNStruct *)context)->pulse->nobs;o1++)CG[o1]=new double[((MNStruct *)context)->Gsize];

	double **GCG= new double*[((MNStruct *)context)->Gsize]; for(int o1=0;o1<((MNStruct *)context)->Gsize;o1++)GCG[o1]=new double[((MNStruct *)context)->Gsize];


	dgemm(CovMatrix,((MNStruct *)context)->GMatrix, CG, ((MNStruct *)context)->pulse->nobs, ((MNStruct *)context)->pulse->nobs, ((MNStruct *)context)->pulse->nobs, ((MNStruct *)context)->Gsize, 'N', 'N');

	dgemm(((MNStruct *)context)->GMatrix,CG, GCG, ((MNStruct *)context)->pulse->nobs, ((MNStruct *)context)->Gsize, ((MNStruct *)context)->pulse->nobs, ((MNStruct *)context)->Gsize, 'T', 'N');

	dgemv(((MNStruct *)context)->GMatrix,WorkRes,GRes,((MNStruct *)context)->pulse->nobs,((MNStruct *)context)->Gsize,'T');

	double covdet=0;
	double *WorkGRes = new double[((MNStruct *)context)->Gsize];
	for(int o1=0;o1<((MNStruct *)context)->Gsize; o1++){
		WorkGRes[o1]=GRes[o1];
	}
	dpotrf(GCG, ((MNStruct *)context)->Gsize, covdet);
        dpotrs(GCG, WorkGRes, ((MNStruct *)context)->Gsize);



	double Chisq=0;


	for(int o1=0;o1<((MNStruct *)context)->Gsize; o1++){
		Chisq += GRes[o1]*WorkGRes[o1];
	}

	if(isnan(covdet) || isinf(covdet) || isnan(Chisq) || isinf(Chisq)){

		lnew=-pow(10.0,200);
// 		printf("red amp and alpha %g %g\n",redamp,redalpha);
// 		printf("Like: %g %g %g \n",lnew,Chisq,covdet);
		
	}
	else{
		lnew = -0.5*(((MNStruct *)context)->Gsize*log(2*M_PI) + covdet + Chisq);	

	}

	delete[] EFAC;
	for(int o=0;o<((MNStruct *)context)->pulse->nobs;o++){delete[] CovMatrix[o];}
	delete[] CovMatrix;
	for(int o=0;o<((MNStruct *)context)->pulse->nobs;o++){delete[] CG[o];}
	delete[] CG;
	for(int o=0;o<((MNStruct *)context)->Gsize;o++){delete[] GCG[o];}
	delete[] GCG;

	delete[] WorkRes;
	delete[] WorkGRes;
	delete[] GRes;
	

}


void WhiteLogLikeMax(double *Cube, int &ndim, int &npars, double &lnew, void *context)
{

	clock_t startClock,endClock;
	long double LDparams[ndim];
	double *EFAC;
	double EQUAD;
	int pcount=0;
// 	printf("here1\n");

// 	printf("here1.5\n");
	for(int p=0;p< ((MNStruct *)context)->numFitTiming + ((MNStruct *)context)->numFitJumps; p++){
		LDparams[p]=Cube[p]*(((MNStruct *)context)->LDpriors[p][1]) + (((MNStruct *)context)->LDpriors[p][0]);
		//printf("CubeTM: %i %g \n", p, Cube[p]); 
	}
// 	printf("here2\n");

	for(int p=0;p<((MNStruct *)context)->numFitTiming;p++){
		((MNStruct *)context)->pulse->param[((MNStruct *)context)->TempoFitNums[p][0]].val[((MNStruct *)context)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)context)->numFitJumps;p++){
		((MNStruct *)context)->pulse->jumpVal[((MNStruct *)context)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}
// 	printf("here3\n");
	if(((MNStruct *)context)->numFitEFAC == 0){
		EFAC=new double[1];
		EFAC[0]=1;
// 		
	}
	else if(((MNStruct *)context)->numFitEFAC == 1){
		EFAC=new double[1];
		EFAC[0]=Cube[pcount];
		pcount++;
	}
	else if(((MNStruct *)context)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)context)->numFitEFAC];
		for(int p=0;p< ((MNStruct *)context)->numFitEFAC; p++){
			EFAC[p]=Cube[pcount];
			//printf("CubeEFAC: %i %g \n", pcount, Cube[pcount]); 
			pcount++;
			
		}
	}				
// 	printf("here4\n");
	if(((MNStruct *)context)->numFitEQUAD == 0){
		EQUAD=0;
// 		printf("E: %g %g\n",EFAC[0],EQUAD);
	}
	else{
		
		EQUAD=pow(10.0,2*Cube[pcount]);
		//printf("CubeEQUAD: %i %g \n", pcount, Cube[pcount]);
		pcount++;
		 

	}
// 	printf("here\n",EFAC[0],EQUAD);
	fastformBatsAll(((MNStruct *)context)->pulse,((MNStruct *)context)->numberpulsars);                /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)context)->pulse,((MNStruct *)context)->numberpulsars,1);       /* Form residuals */




	double Chisq=0;
	double noiseval=0;
	double detN=0;
	for(int o=0;o<((MNStruct *)context)->pulse->nobs; o++){
		noiseval=pow(((((MNStruct *)context)->pulse->obsn[o].toaErr)*pow(10.0,-6))*EFAC[((MNStruct *)context)->sysFlags[o]],2) + EQUAD;
		Chisq += pow((((MNStruct *)context)->pulse->obsn[o].residual),2)/noiseval;
		detN += log(noiseval);
// 		printf("detn: %g %g \n",noiseval,detN);
	}

	if(isnan(detN) || isinf(detN) || isnan(Chisq) || isinf(Chisq)){

		lnew=-pow(10.0,200);
// 		printf("red amp and alpha %g %g\n",redamp,redalpha);
// 		printf("Like: %g %g %g \n",lnew,Chisq,covdet);
		
	}
	else{
		lnew = -0.5*(((MNStruct *)context)->pulse->nobs*log(2*M_PI) + detN + Chisq);	
// 		printf("red amp and alpha %g %g\n",redamp,redalpha);
// 		printf("Like: %g %g %g \n",lnew,Chisq,covdet);

	}

	delete[] EFAC;


}


void WhiteMarginLogLikeMax(double *Cube, int &ndim, int &npars, double &lnew, void *context)
{

	clock_t startClock,endClock;
	long double LDparams[ndim];
	double *EFAC;
	double EQUAD;
	int pcount=0;



	for(int p=0;p< ((MNStruct *)context)->numFitTiming + ((MNStruct *)context)->numFitJumps; p++){
		LDparams[p]=Cube[p]*(((MNStruct *)context)->LDpriors[p][1]) + (((MNStruct *)context)->LDpriors[p][0]);
	}


	for(int p=0;p<((MNStruct *)context)->numFitTiming;p++){
		((MNStruct *)context)->pulse->param[((MNStruct *)context)->TempoFitNums[p][0]].val[((MNStruct *)context)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)context)->numFitJumps;p++){
		((MNStruct *)context)->pulse->jumpVal[((MNStruct *)context)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	if(((MNStruct *)context)->numFitEFAC == 0){
		EFAC=new double[1];
		EFAC[0]=1;
// 		
	}
	else if(((MNStruct *)context)->numFitEFAC == 1){
		EFAC=new double[1];
		EFAC[0]=Cube[pcount];
		pcount++;
	}
	else if(((MNStruct *)context)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)context)->numFitEFAC];
		for(int p=0;p< ((MNStruct *)context)->numFitEFAC; p++){
			EFAC[p]=Cube[pcount];
			pcount++;
		}
	}				

	if(((MNStruct *)context)->numFitEQUAD == 0){
		EQUAD=0;
// 		printf("E: %g %g\n",EFAC[0],EQUAD);
	}
	else{
		
		EQUAD=pow(10.0,2*Cube[pcount]);
		pcount++;

	}

	fastformBatsAll(((MNStruct *)context)->pulse,((MNStruct *)context)->numberpulsars);                /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)context)->pulse,((MNStruct *)context)->numberpulsars,1);       /* Form residuals */





	double *Noise=new double[((MNStruct *)context)->pulse->nobs];
	double *Res=new double[((MNStruct *)context)->pulse->nobs];
	double *GRes=new double[((MNStruct *)context)->Gsize];

	for(int o=0;o<((MNStruct *)context)->pulse->nobs; o++){
		Noise[o]=pow(((((MNStruct *)context)->pulse->obsn[o].toaErr)*pow(10.0,-6))*EFAC[((MNStruct *)context)->sysFlags[o]],2) + EQUAD;
		Res[o]=((MNStruct *)context)->pulse->obsn[o].residual;
// 		printf("%i %g %g \n",o,Noise[o],Res[o]);
	}

	double **NG = new double*[((MNStruct *)context)->pulse->nobs]; for (int k=0; k<((MNStruct *)context)->pulse->nobs; k++) NG[k] = new double[((MNStruct *)context)->Gsize];
	for(int i=0;i<((MNStruct *)context)->pulse->nobs;i++){
		for(int j=0;j<((MNStruct *)context)->Gsize; j++){
			NG[i][j]=((MNStruct *)context)->GMatrix[i][j]*Noise[i];
// 			printf("%i %i %g %g %g \n",i,j,((MNStruct *)context)->GMatrix[i][j],Noise[i],NG[i][j]);

		}
	}

	double** GG = new double*[((MNStruct *)context)->Gsize]; for (int k=0; k<((MNStruct *)context)->Gsize; k++) GG[k] = new double[((MNStruct *)context)->Gsize];

	dgemm(((MNStruct *)context)->GMatrix, NG,GG,((MNStruct *)context)->pulse->nobs, ((MNStruct *)context)->Gsize,((MNStruct *)context)->pulse->nobs, ((MNStruct *)context)->Gsize, 'T','N');


	dgemv(((MNStruct *)context)->GMatrix,Res,GRes,((MNStruct *)context)->pulse->nobs,((MNStruct *)context)->Gsize,'T');



	double det=0;
	double *WorkGRes = new double[((MNStruct *)context)->Gsize];
	for(int o1=0;o1<((MNStruct *)context)->Gsize; o1++){
		WorkGRes[o1]=GRes[o1];
	}
	dpotrf(GG, ((MNStruct *)context)->Gsize, det);
        dpotrs(GG, WorkGRes, ((MNStruct *)context)->Gsize);



	double Chisq=0;
	for(int o1=0;o1<((MNStruct *)context)->Gsize; o1++){
		Chisq += GRes[o1]*WorkGRes[o1];
	}



	if(isnan(det) || isinf(det) || isnan(Chisq) || isinf(Chisq)){

		lnew=-pow(10.0,200);
// 		printf("red amp and alpha %g %g\n",redamp,redalpha);
// 		printf("Like: %g %g %g \n",lnew,Chisq,covdet);
		
	}
	else{
		lnew = -0.5*(((MNStruct *)context)->Gsize*log(2.0*M_PI) + det + Chisq);	
// 		printf("red amp and alpha %g %g\n",redamp,redalpha);
// 		printf("Like: %g %g %g \n",lnew,Chisq,det);

	}

	delete[] EFAC;
	delete[] Noise;
	delete[] Res;
	delete[] GRes;
	delete[] WorkGRes;

	for (int j = 0; j < ((MNStruct *)context)->pulse->nobs; j++){
		delete[] NG[j];
	}
	delete[] NG;

	for (int j = 0; j < ((MNStruct *)context)->Gsize; j++){
		delete[] GG[j];
	}
	delete[] GG;

	
	//printf("Chisq: %g %g \n",det, Chisq);

}


void LRedLogLikeMax(double *Cube, int &ndim, int &npars, double &lnew, void *context)
{

	clock_t startClock,endClock;
	long double LDparams[ndim];
	double *EFAC;
	double EQUAD;
	int pcount=0;


	for(int p=0;p< ((MNStruct *)context)->numFitTiming + ((MNStruct *)context)->numFitJumps; p++){
		LDparams[p]=Cube[p]*(((MNStruct *)context)->LDpriors[p][1]) + (((MNStruct *)context)->LDpriors[p][0]);
		//printf("LP: %i %g \n",p,Cube[p]);
	}


	for(int p=0;p<((MNStruct *)context)->numFitTiming;p++){
		((MNStruct *)context)->pulse->param[((MNStruct *)context)->TempoFitNums[p][0]].val[((MNStruct *)context)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)context)->numFitJumps;p++){
		((MNStruct *)context)->pulse->jumpVal[((MNStruct *)context)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	if(((MNStruct *)context)->numFitEFAC == 0){
		EFAC=new double[1];
		EFAC[0]=1;
// 		
	}
	else if(((MNStruct *)context)->numFitEFAC == 1){
		EFAC=new double[1];
		EFAC[0]=Cube[pcount];
		pcount++;
	}
	else if(((MNStruct *)context)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)context)->numFitEFAC];
		for(int p=0;p< ((MNStruct *)context)->numFitEFAC; p++){
			EFAC[p]=Cube[pcount];
			pcount++;
		}
	}				

	if(((MNStruct *)context)->numFitEQUAD == 0){
		EQUAD=0;
// 		printf("EQUAD: %g \n",EQUAD);
	}
	else{
		
		EQUAD=pow(10.0,2*Cube[pcount]);
		pcount++;
// 		printf("EQUAD: %g %g %g %i \n",EQUAD,EQUADPrior[0],EQUADPrior[1],((MNStruct *)context)->numFitTiming + ((MNStruct *)context)->numFitJumps + ((MNStruct *)context)->numFitEFAC);
	}

  	

// 	startClock = clock();
	
	
	fastformBatsAll(((MNStruct *)context)->pulse,((MNStruct *)context)->numberpulsars);                /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)context)->pulse,((MNStruct *)context)->numberpulsars,1);       /* Form residuals */

	int FitCoeff=2*(((MNStruct *)context)->numFitRedCoeff);
	double *WorkNoise=new double[((MNStruct *)context)->pulse->nobs];
	double *powercoeff=new double[FitCoeff];


	double tdet=0;
	double timelike=0;


	double *resvec=new double[((MNStruct *)context)->pulse->nobs];
	for(int o=0;o<((MNStruct *)context)->pulse->nobs; o++){
		//	printf("Noise %i %g %g %g\n",m1,Noise[m1],EFAC[flagList[m1]],EQUAD);
			WorkNoise[o]=pow(((((MNStruct *)context)->pulse->obsn[o].toaErr)*pow(10.0,-6))*EFAC[((MNStruct *)context)->sysFlags[o]],2) + EQUAD;
			
			tdet=tdet+log(WorkNoise[o]);
			WorkNoise[o]=1.0/WorkNoise[o];
			timelike=timelike+pow((((MNStruct *)context)->pulse->obsn[o].residual),2)*WorkNoise[o];
			resvec[o]=((MNStruct *)context)->pulse->obsn[o].residual;
	}

	double *NFd = new double[FitCoeff];
	double **FMatrix=new double*[((MNStruct *)context)->pulse->nobs];
	for(int i=0;i<((MNStruct *)context)->pulse->nobs;i++){
		FMatrix[i]=new double[FitCoeff];
	}

	double **NF=new double*[((MNStruct *)context)->pulse->nobs];
	for(int i=0;i<((MNStruct *)context)->pulse->nobs;i++){
		NF[i]=new double[FitCoeff];
	}

	double **FNF=new double*[FitCoeff];
	for(int i=0;i<FitCoeff;i++){
		FNF[i]=new double[FitCoeff];
	}





	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)context)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)context)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)context)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)context)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)context)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)context)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)context)->pulse->obsn[i].bat;
		  }
	      }
	  }
// 	printf("Total time span = %.6f days = %.6f years\n",end-start,(end-start)/365.25);
	double maxtspan=end-start;


	double freqdet=0;
	for (int i=0; i<FitCoeff/2; i++){
		int pnum=pcount;
		double pc=Cube[pcount];
		
		powercoeff[i]=pow(10.0,pc)/(maxtspan*24*60*60);///(365.25*24*60*60)/4;
		powercoeff[i+FitCoeff/2]=powercoeff[i];
		freqdet=freqdet+2*log(powercoeff[i]);
		pcount++;
	}


	int coeffsize=FitCoeff/2;
	std::vector<double>freqs(FitCoeff/2);
	for(int i=0;i<FitCoeff/2;i++){
		freqs[i]=double(i+1)/maxtspan;
	}	
	

	for(int i=0;i<FitCoeff/2;i++){
		for(int k=0;k<((MNStruct *)context)->pulse->nobs;k++){
			
			FMatrix[k][i]=cos(2*M_PI*freqs[i]*((double)((MNStruct *)context)->pulse->obsn[k].bat));
// 			printf("cos %i %i %g \n",i,k,FMatrix[k][i]);
		}
	}

	for(int i=0;i<FitCoeff/2;i++){
		for(int k=0;k<((MNStruct *)context)->pulse->nobs;k++){
			FMatrix[k][i+FitCoeff/2]=sin(2*M_PI*freqs[i]*((double)((MNStruct *)context)->pulse->obsn[k].bat));
// 			printf("sin %i %i %g \n",i+FitCoeff/2,k,FMatrix[k][i+FitCoeff/2]);
		}
	}





// 	makeFourier(((MNStruct *)context)->pulse, FitCoeff, FMatrix);

	for(int i=0;i<((MNStruct *)context)->pulse->nobs;i++){
		for(int j=0;j<FitCoeff;j++){
// 			printf("%i %i %g %g \n",i,j,WorkNoise[i],FMatrix[i][j]);
			NF[i][j]=WorkNoise[i]*FMatrix[i][j];
		}
	}
	dgemv(NF,resvec,NFd,((MNStruct *)context)->pulse->nobs,FitCoeff,'T');
	dgemm(FMatrix, NF , FNF, ((MNStruct *)context)->pulse->nobs, FitCoeff, ((MNStruct *)context)->pulse->nobs, FitCoeff, 'T', 'N');


	double **PPFM=new double*[FitCoeff];
	for(int i=0;i<FitCoeff;i++){
		PPFM[i]=new double[FitCoeff];
		for(int j=0;j<FitCoeff;j++){
			PPFM[i][j]=0;
		}
	}


	for(int c1=0; c1<FitCoeff; c1++){
		PPFM[c1][c1]=1.0/powercoeff[c1];
	}



	for(int j=0;j<FitCoeff;j++){
		for(int k=0;k<FitCoeff;k++){
// 			printf("%i %i %g %g \n",j,k,PPFM[j][k],FNF[j][k]);
			PPFM[j][k]=PPFM[j][k]+FNF[j][k];
		}
	}

 	
	double jointdet=0;
	dpotrf(PPFM, FitCoeff, jointdet);
        dpotri(PPFM,FitCoeff);

	double freqlike=0;
	for(int i=0;i<FitCoeff;i++){
		for(int j=0;j<FitCoeff;j++){
			//sprintf("%i %i %g %g\n",i,j,NFd[i],PPFM[i][j]);
			freqlike=freqlike+NFd[i]*PPFM[i][j]*NFd[j];
		}
	}
	
	lnew=-0.5*(jointdet+tdet+freqdet+timelike-freqlike);

	if(isnan(lnew) || isinf(lnew)){

		lnew=-pow(10.0,200);
// 		printf("red amp and alpha %g %g\n",redamp,redalpha);
// 		printf("Like: %g %g %g \n",lnew,Chisq,covdet);
		
	}


	delete[] EFAC;
	delete[] WorkNoise;
	delete[] powercoeff;
	delete[] resvec;
	delete[] NFd;

	for (int j = 0; j < FitCoeff; j++){
		delete[] PPFM[j];
	}
	delete[] PPFM;

	for (int j = 0; j < ((MNStruct *)context)->pulse->nobs; j++){
		delete[] NF[j];
	}
	delete[] NF;

	for (int j = 0; j < FitCoeff; j++){
		delete[] FNF[j];
	}
	delete[] FNF;

	for (int j = 0; j < ((MNStruct *)context)->pulse->nobs; j++){
		delete[] FMatrix[j];
	}
	delete[] FMatrix;

// 	if(isinf(lnew) || isinf(jointdet) || isinf(tdet) || isinf(freqdet) || isinf(timelike) || isinf(freqlike)){
// 	printf("Chisq: %g %g %g %g %g %g \n",lnew,jointdet,tdet,freqdet,timelike,freqlike);
// 	}

}

void LRedMarginLogLikeMax(double *Cube, int &ndim, int &npars, double &lnew, void *context)
{

	clock_t startClock,endClock;
	long double LDparams[ndim];
	double *EFAC;
	double EQUAD;
	int pcount=0;


	for(int p=0;p< ((MNStruct *)context)->numFitTiming + ((MNStruct *)context)->numFitJumps; p++){
		LDparams[p]=Cube[p]*(((MNStruct *)context)->LDpriors[p][1]) + (((MNStruct *)context)->LDpriors[p][0]);
	}


	for(int p=0;p<((MNStruct *)context)->numFitTiming;p++){
		((MNStruct *)context)->pulse->param[((MNStruct *)context)->TempoFitNums[p][0]].val[((MNStruct *)context)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)context)->numFitJumps;p++){
		((MNStruct *)context)->pulse->jumpVal[((MNStruct *)context)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	if(((MNStruct *)context)->numFitEFAC == 0){
		EFAC=new double[1];
		EFAC[0]=1;
// 		
	}
	else if(((MNStruct *)context)->numFitEFAC == 1){
		EFAC=new double[1];
		EFAC[0]=Cube[pcount];
		pcount++;
	}
	else if(((MNStruct *)context)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)context)->numFitEFAC];
		for(int p=0;p< ((MNStruct *)context)->numFitEFAC; p++){
			EFAC[p]=Cube[pcount];
			pcount++;
		}
	}				

	if(((MNStruct *)context)->numFitEQUAD == 0){
		EQUAD=0;
// 		printf("EQUAD: %g \n",EQUAD);
	}
	else{
		
		EQUAD=pow(10.0,2*Cube[pcount]);
		pcount++;
// 		printf("EQUAD: %g %g %g %i \n",EQUAD,EQUADPrior[0],EQUADPrior[1],((MNStruct *)context)->numFitTiming + ((MNStruct *)context)->numFitJumps + ((MNStruct *)context)->numFitEFAC);
	}

  	

// 	startClock = clock();
	
	
	fastformBatsAll(((MNStruct *)context)->pulse,((MNStruct *)context)->numberpulsars);                /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)context)->pulse,((MNStruct *)context)->numberpulsars,1);       /* Form residuals */

	int FitCoeff=2*(((MNStruct *)context)->numFitRedCoeff);
	double *powercoeff=new double[FitCoeff];

	double *Noise=new double[((MNStruct *)context)->pulse->nobs];
	double *Res=new double[((MNStruct *)context)->pulse->nobs];
	double *GRes=new double[((MNStruct *)context)->Gsize];

	for(int o=0;o<((MNStruct *)context)->pulse->nobs; o++){
		Noise[o]=pow(((((MNStruct *)context)->pulse->obsn[o].toaErr)*pow(10.0,-6))*EFAC[((MNStruct *)context)->sysFlags[o]],2) + EQUAD;
		Res[o]=((MNStruct *)context)->pulse->obsn[o].residual;
	}

	double **NG = new double*[((MNStruct *)context)->pulse->nobs]; for (int k=0; k<((MNStruct *)context)->pulse->nobs; k++) NG[k] = new double[((MNStruct *)context)->Gsize];
	for(int i=0;i<((MNStruct *)context)->pulse->nobs;i++){
		for(int j=0;j<((MNStruct *)context)->Gsize; j++){
			NG[i][j]=((MNStruct *)context)->GMatrix[i][j]*Noise[i];

		}
	}

	double** GG = new double*[((MNStruct *)context)->Gsize]; for (int k=0; k<((MNStruct *)context)->Gsize; k++) GG[k] = new double[((MNStruct *)context)->Gsize];

	dgemm(((MNStruct *)context)->GMatrix, NG,GG,((MNStruct *)context)->pulse->nobs, ((MNStruct *)context)->Gsize,((MNStruct *)context)->pulse->nobs, ((MNStruct *)context)->Gsize, 'T','N');

	
	double tdet=0;
	dpotrf(GG, ((MNStruct *)context)->Gsize, tdet);
	dpotri(GG,((MNStruct *)context)->Gsize);

	dgemm(((MNStruct *)context)->GMatrix, GG,NG,((MNStruct *)context)->pulse->nobs, ((MNStruct *)context)->Gsize, ((MNStruct *)context)->Gsize, ((MNStruct *)context)->Gsize, 'N','N');

	double **GNG = new double*[((MNStruct *)context)->pulse->nobs]; for (int k=0; k<((MNStruct *)context)->pulse->nobs; k++) GNG[k] = new double[((MNStruct *)context)->pulse->nobs];	

	dgemm(NG, ((MNStruct *)context)->GMatrix, GNG,((MNStruct *)context)->pulse->nobs, ((MNStruct *)context)->Gsize, ((MNStruct *)context)->pulse->nobs, ((MNStruct *)context)->Gsize, 'N','T');


	double timelike=0;
	for(int o1=0; o1<((MNStruct *)context)->pulse->nobs; o1++){
		for(int o2=0;o2<((MNStruct *)context)->pulse->nobs; o2++){
			timelike=timelike+Res[o1]*GNG[o1][o2]*Res[o2];
		}
	}

	double *NFd = new double[FitCoeff];
	double **FMatrix=new double*[((MNStruct *)context)->pulse->nobs];
	for(int i=0;i<((MNStruct *)context)->pulse->nobs;i++){
		FMatrix[i]=new double[FitCoeff];
	}

	double **NF=new double*[((MNStruct *)context)->pulse->nobs];
	for(int i=0;i<((MNStruct *)context)->pulse->nobs;i++){
		NF[i]=new double[FitCoeff];
	}

	double **FNF=new double*[FitCoeff];
	for(int i=0;i<FitCoeff;i++){
		FNF[i]=new double[FitCoeff];
	}

	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)context)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)context)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)context)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)context)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)context)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)context)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)context)->pulse->obsn[i].bat;
		  }
	      }
	  }
// 	printf("Total time span = %.6f days = %.6f years\n",end-start,(end-start)/365.25);
	double maxtspan=end-start;


	double freqdet=0;
	for (int i=0; i<FitCoeff/2; i++){
		int pnum=pcount;
		double pc=Cube[pcount];
		
		powercoeff[i]=pow(10.0,pc)/(maxtspan*24*60*60);///(365.25*24*60*60)/4;
		powercoeff[i+FitCoeff/2]=powercoeff[i];
		freqdet=freqdet+2*log(powercoeff[i]);
		pcount++;
	}


	int coeffsize=FitCoeff/2;
	std::vector<double>freqs(FitCoeff/2);
	for(int i=0;i<FitCoeff/2;i++){
		freqs[i]=double(i+1)/maxtspan;
	}	
	

	for(int i=0;i<FitCoeff/2;i++){
		for(int k=0;k<((MNStruct *)context)->pulse->nobs;k++){
			double time=(double)((MNStruct *)context)->pulse->obsn[k].bat; //- (double)((MNStruct *)context)->pulse->param[param_pepoch].val[0] - maxtspan/2;
			FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);
// 			printf("cos %i %i %g \n",i,k,time);
		}
	}

	for(int i=0;i<FitCoeff/2;i++){
		for(int k=0;k<((MNStruct *)context)->pulse->nobs;k++){
			double time=(double)((MNStruct *)context)->pulse->obsn[k].bat; //- (double)((MNStruct *)context)->pulse->param[param_pepoch].val[0] - maxtspan/2;
			FMatrix[k][i+FitCoeff/2]=sin(2*M_PI*freqs[i]*time);
// 			printf("sin %i %i %g \n",i+FitCoeff/2,k,time);
		}
	}


	dgemm(GNG, FMatrix , NF, ((MNStruct *)context)->pulse->nobs,((MNStruct *)context)->pulse->nobs, ((MNStruct *)context)->pulse->nobs, FitCoeff, 'N', 'N');

	dgemm(FMatrix, NF , FNF, ((MNStruct *)context)->pulse->nobs, FitCoeff, ((MNStruct *)context)->pulse->nobs, FitCoeff, 'T', 'N');

	dgemv(NF,Res,NFd,((MNStruct *)context)->pulse->nobs,FitCoeff,'T');

	double **PPFM=new double*[FitCoeff];
	for(int i=0;i<FitCoeff;i++){
		PPFM[i]=new double[FitCoeff];
		for(int j=0;j<FitCoeff;j++){
			PPFM[i][j]=0;
		}
	}


	for(int c1=0; c1<FitCoeff; c1++){
		PPFM[c1][c1]=1.0/powercoeff[c1];
	}



	for(int j=0;j<FitCoeff;j++){
		for(int k=0;k<FitCoeff;k++){
			
			PPFM[j][k]=PPFM[j][k]+FNF[j][k];
		}
	}

 	
	double jointdet=0;
	dpotrf(PPFM, FitCoeff, jointdet);
        dpotri(PPFM,FitCoeff);

	double freqlike=0;
	for(int i=0;i<FitCoeff;i++){
		for(int j=0;j<FitCoeff;j++){
// 			printf("%i %i %g %g\n",i,j,NFd[i],PPFM[i][j]);
			freqlike=freqlike+NFd[i]*PPFM[i][j]*NFd[j];
		}
	}
	
	lnew=-0.5*(tdet+jointdet+freqdet+timelike-freqlike);

	if(isnan(lnew) || isinf(lnew)){

		lnew=-pow(10.0,200);
// 		printf("red amp and alpha %g %g\n",redamp,redalpha);
// 		printf("Like: %g %g %g \n",lnew,Chisq,covdet);
		
	}


	delete[] EFAC;
	delete[] powercoeff;
	delete[] NFd;

	for (int j = 0; j < FitCoeff; j++){
		delete[]PPFM[j];
	}
	delete[]PPFM;

	for (int j = 0; j < ((MNStruct *)context)->pulse->nobs; j++){
		delete[]NF[j];
	}
	delete[]NF;

	for (int j = 0; j < FitCoeff; j++){
		delete[]FNF[j];
	}
	delete[]FNF;

	for (int j = 0; j < ((MNStruct *)context)->pulse->nobs; j++){
		delete[]FMatrix[j];
	}
	delete[]FMatrix;

	delete[] Noise;
	delete[] Res;
	delete[] GRes;

	for (int j = 0; j < ((MNStruct *)context)->pulse->nobs; j++){
		delete[] NG[j];
	}
	delete[] NG;

	for (int j = 0; j < ((MNStruct *)context)->Gsize; j++){
		delete[]GG[j];
	}
	delete[] GG;

	for (int j = 0; j < ((MNStruct *)context)->pulse->nobs; j++){
		delete[] GNG[j];
	}
	delete[] GNG;

// 	if(isinf(lnew) || isinf(jointdet) || isinf(tdet) || isinf(freqdet) || isinf(timelike) || isinf(freqlike)){
// 	printf("Chisq: %g %g %g %g %g %g \n",lnew,jointdet,tdet,freqdet,timelike,freqlike);
// 	}

}


/* The negative likelihood function in GSL understandable language
 *
 * Input
 * pvP:	 	Parameters of the likelihood function
 * params:	Extra information. Here a pointer to the model object
 *
 * Output
 *
 * Return:	Log-Likelihood value
 * */
double negloglikelihood(const gsl_vector *pvP, void *context) {

	int ndims=((MNStruct *)context)->numdims;
	double pdParameters[ndims];
	int nParameters;
	double lval;

	
	
	// Obtain the pointer to the model
	
	for(int i=0; i<ndims; i++) {
		pdParameters[i] = gsl_vector_get(pvP, i);
  	} // for i

	if(((MNStruct *)context)->JumpMargin != 0 || ((MNStruct *)context)->TimeMargin != 0 ){
		//printf("Margin \n");
		if(((MNStruct *)context)->incRED==0){
			WhiteLogLikeMax(pdParameters, ndims, ndims, lval, context);
		}
		else if(((MNStruct *)context)->incRED==1){
			vHRedLogLikeMax(pdParameters, ndims, ndims, lval, context);
		}
		else if(((MNStruct *)context)->incRED==2){
			LRedLogLikeMax(pdParameters, ndims, ndims, lval, context);
		}
	}

	else if(((MNStruct *)context)->JumpMargin == 0 && ((MNStruct *)context)->TimeMargin == 0 ){
		//printf("No Margin \n");
		if(((MNStruct *)context)->incRED==0){
			WhiteLogLikeMax(pdParameters, ndims, ndims, lval, context);
		}
		else if(((MNStruct *)context)->incRED==1){
			vHRedLogLikeMax(pdParameters, ndims, ndims, lval, context);
		}
		else if(((MNStruct *)context)->incRED==2){
			LRedLogLikeMax(pdParameters, ndims, ndims, lval, context);
		}
	}
	
	//printf("Like: %g \n",lval);
	// The loglikelihood function is virtual, so the correct one is called
	return -lval;
} // loglikelihood

/* This function finds the maximum likelihood parameters for the stochastic
 * signal (here power-law power spectral density with a white high-frequency
 * tail)
 *
 * Input
 * nParameters:		The number of stochastic signal parameters
 * 
 * Output:
 * pdParameters:	The maximum likelihood value of the parameters
 * */
void NelderMeadOptimum(int nParameters, long double *LdParameters, void *context) {

	printf("\n\n Performing Minimisation over current model parameters \n\n");

  const gsl_multimin_fminimizer_type *pmtMinimiserType = gsl_multimin_fminimizer_nmsimplex;
  gsl_multimin_fminimizer *pmMinimiser = NULL;
  gsl_vector *vStepSize, *vStart;
  gsl_multimin_function MinexFunc;

  double *pdParameterEstimates;
  double dSize;
  int nIteration=0, nStatus;

  pdParameterEstimates = new double[nParameters];
  vStepSize = gsl_vector_alloc(nParameters);
  vStart = gsl_vector_alloc(nParameters);

	int pcount=0;
  // Obtain the starting point as rough parameter estimates
  for(int i=0; i<((MNStruct *)context)->numFitTiming+((MNStruct *)context)->numFitJumps; i++) {
    gsl_vector_set(vStepSize, pcount, 10.0);
    gsl_vector_set(vStart, pcount, 0.0);
	pcount++;
  } // for i

  for(int i =0; i< ((MNStruct *)context)->numFitEFAC; i++){
		//printf("Setting EFAC: %i\n",i);
	    gsl_vector_set(vStepSize, pcount, 1.0);
   	    gsl_vector_set(vStart, pcount, 2.0);
		pcount++;
	}
  for(int i =0; i< ((MNStruct *)context)->numFitEQUAD; i++){
		//printf("Setting EQUD: %i\n",i);
	    gsl_vector_set(vStepSize, pcount, 0.5);
   	    gsl_vector_set(vStart, pcount, -5.0);
		pcount++;
	}
	if(((MNStruct *)context)->incRED == 1){
		//printf("Setting Red\n");
		gsl_vector_set(vStart, pcount, -12.0);
		gsl_vector_set(vStepSize, pcount, 0.5);
		pcount++;
		gsl_vector_set(vStart, pcount, 3.1);
		gsl_vector_set(vStepSize, pcount, 0.25);
		pcount++;
	}

	if(((MNStruct *)context)->incRED == 2){
		for(int i =0; i< ((MNStruct *)context)->numFitRedCoeff; i++){
			gsl_vector_set(vStepSize, pcount, 0.5);
			gsl_vector_set(vStart, pcount, -3.0);
			pcount++;
		}
	}	

  // Initial GSL vector of the vertex sizes, and the starting point



  // Initialise the iteration procedure
  MinexFunc.f = &negloglikelihood;
  MinexFunc.n = nParameters;
  MinexFunc.params = context;
  pmMinimiser = gsl_multimin_fminimizer_alloc(pmtMinimiserType, nParameters);
  gsl_multimin_fminimizer_set(pmMinimiser, &MinexFunc, vStart, vStepSize);

  // Iterate to the maximum likelihood
  do {
    nIteration++;
    nStatus = gsl_multimin_fminimizer_iterate(pmMinimiser);

    if(nStatus) break;

    // Check whether we are close enough to the minimum (1e-3 error)
    dSize = gsl_multimin_fminimizer_size(pmMinimiser);
    nStatus = gsl_multimin_test_size(dSize, 1e-3);

      for(int i=0; i<nParameters; i++) {
	pdParameterEstimates[i] = gsl_vector_get(pmMinimiser->x, i);
	//printf("%i %g \n", i,pdParameters[i]);
      } // for i

    if(nStatus == GSL_SUCCESS) {
      fprintf(stderr, "Converged to maximum likelihood with downhill simplex                               \n");
    } // if nStatus

    // Print iteration values
	if(nIteration % 100 ==0){
    		printf("Step[%i]: Convergence: %g Current Minimum: %g \n", nIteration,dSize,gsl_multimin_fminimizer_minimum(pmMinimiser));
	}
  } while (nStatus == GSL_CONTINUE);


	printf("\n");

	pcount=0;
	for(int j=0;j<((MNStruct *)context)->numFitTiming;j++){
	
		long double value=pdParameterEstimates[j]*(((MNStruct *)context)->LDpriors[j][1])+(((MNStruct *)context)->LDpriors[j][0]);
		LdParameters[pcount]=value;
		printf("   Max %s : %.20Lg \n", ((MNStruct *)context)->pulse->param[((MNStruct *)context)->TempoFitNums[pcount][0]].shortlabel[((MNStruct *)context)->TempoFitNums[pcount][1]], value);
		pcount++;
	}

	for(int j=0;j<((MNStruct *)context)->numFitJumps;j++){

		long double value=pdParameterEstimates[pcount]*(((MNStruct *)context)->LDpriors[pcount][1])+(((MNStruct *)context)->LDpriors[pcount][0]);
		LdParameters[pcount]=value;
		printf("   Max Jump %i :  %.10Lg \n", j,value);
		pcount++;
	}
	
	  for(int i =0; i< ((MNStruct *)context)->numFitEFAC; i++){
		printf("   Max EFAC %i :  %.10g \n", i,pdParameterEstimates[pcount]);
		pcount++;
	}
  for(int i =0; i< ((MNStruct *)context)->numFitEQUAD; i++){
		printf("   Max EQUAD %i :  %.10g \n", i,pdParameterEstimates[pcount]);
		pcount++;
	}
	if(((MNStruct *)context)->incRED == 1){
		printf("   Max Red Amp :  %.10g \n", pdParameterEstimates[pcount]);
		pcount++;
		printf("   Max Red Index :  %.10g \n",pdParameterEstimates[pcount]);
		pcount++;
	}

	if(((MNStruct *)context)->incRED == 2){
		for(int i =0; i< ((MNStruct *)context)->numFitRedCoeff; i++){
			printf("   Max Red Coeff %i :  %.10g \n", i,pdParameterEstimates[pcount]);
			pcount++;
		}
	}	



  gsl_vector_free(vStart);
  gsl_vector_free(vStepSize);
  gsl_multimin_fminimizer_free(pmMinimiser);
  delete[] pdParameterEstimates;
} // NelderMeadOptimum

