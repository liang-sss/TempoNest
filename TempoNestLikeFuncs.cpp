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


#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <vector>
#include <gsl/gsl_sf_gamma.h>
#include "dgemm.h"
#include "dgemv.h"
#include "dpotri.h"
#include "dpotrf.h"
#include "dpotrs.h"
#include "tempo2.h"
#include "TempoNest.h"
#include "dgesvd.h"
#include "qrdecomp.h"
#include "cholesky.h"
#include "T2toolkit.h"
//#include <mpack/mblas_dd.h>
//#include <mpack/mlapack_dd.h>
#include <fstream>
#include <unistd.h>
//#include <mpack/mblas_qd.h>
//#include <mpack/mlapack_qd.h>
#include <sstream>
#include <iomanip>
#include <sstream>
#include <iterator>

void *globalcontext;

void assigncontext(void *context){
        globalcontext=context;
}

void TNothpl(int n,double x,double *pl){


        double a=2.0;
        double b=0.0;
        double c=1.0;
        double y0=1.0;
        double y1=2.0*x;
        pl[0]=1.0;
        pl[1]=2.0*x;


//	printf("I AM IN OTHPL %i \n", n);
        for(int k=2;k<n;k++){

                double c=2.0*(k-1.0);
//		printf("%i %g\n", k, sqrt(double(k*1.0)));
		y0=y0/sqrt(double(k*1.0));
		y1=y1/sqrt(double(k*1.0));
                double yn=(a*x+b)*y1-c*y0;
		yn=yn;///sqrt(double(k));
                pl[k]=yn;///sqrt(double(k));
                y0=y1;
                y1=yn;

        }



}

void outputProfile(int ndim){

        int number_of_lines = 0;
        double weightsum=0;

	std::string longname="/home/ltl21/scratch/Pulsars/ProfileData/J1909-10cm/results/AllTOA-40Shape-EFJitter2-J1909-3744-";

        std::ifstream checkfile;
        std::string checkname = longname+".txt";
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        printf("number of lines %i \n",number_of_lines);
        checkfile.close();

        std::ifstream summaryfile;
        std::string fname = longname+".txt";
        summaryfile.open(fname.c_str());



        printf("Writing Profiles \n");

	double *shapecoeff=new double[((MNStruct *)globalcontext)->numshapecoeff];
	int otherdims =  ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps + ((MNStruct *)globalcontext)->numFitEQUAD + ((MNStruct *)globalcontext)->numFitEFAC;

	int Nbins=1000;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	double **Hermitepoly=new double*[Nbins];
	for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
	double *shapevec=new double[Nbins];

	std::ofstream profilefile;
	std::string dname = longname+"Profiles.txt";
	
	profilefile.open(dname.c_str());

	double *MeanProfile = new double[Nbins];
	double *ProfileErr = new double[Nbins];

	for(int i=0;i<Nbins;i++){
		MeanProfile[i] = 0;
		ProfileErr[i] = 0;
	}
	

	printf("getting mean \n");
	weightsum=0;
        for(int i=0;i<number_of_lines;i++){


		if(i%100 == 0)printf("line: %i of %i %g \n", i, number_of_lines, double(i)/double(number_of_lines));

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);

		double weight=paramlist[0];

                weightsum += weight;
                double like = paramlist[1];

		int cnum=0;
                for(int i = otherdims; i < otherdims+((MNStruct *)globalcontext)->numshapecoeff; i++){
			shapecoeff[cnum] = paramlist[i+2];
			//printf("coeff %i %g \n", cnum, shapecoeff[cnum]);
			cnum++;
			
                }

		double beta = paramlist[otherdims+((MNStruct *)globalcontext)->numshapecoeff+2];

		for(int j =0; j < Nbins; j++){
	

			double timeinterval = 0.0001;
			double time = -1.0*double(Nbins)*timeinterval/2 + j*timeinterval;

			


			TNothpl(numcoeff,time/beta,Hermitepoly[j]);
			for(int k =0; k < numcoeff; k++){
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
				Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*time*time/beta/beta);
			}

		}

		shapecoeff[0] = 1.0;
		dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');

		//profilefile << weightsum;
		//profilefile << " ";


		for(int j = 0; j < Nbins; j++){

			//profilefile << shapevec[j];
			//profilefile << " ";
			//printf("shape %i %g \n", j, shapevec[j]);

			MeanProfile[j] = MeanProfile[j] + shapevec[j]*weight;

		}

		//profilefile << "\n";	

	}

	for(int j = 0; j < Nbins; j++){
		MeanProfile[j] = MeanProfile[j]/weightsum;
	}


	summaryfile.close();

	summaryfile.open(fname.c_str());

	printf("Getting Errors \n");


	weightsum=0;
	 for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);

		double weight=paramlist[0];

                weightsum += weight;
                double like = paramlist[1];

		int cnum=0;
                for(int i = otherdims; i < otherdims+((MNStruct *)globalcontext)->numshapecoeff; i++){
			shapecoeff[cnum] = paramlist[i+2];
			//printf("coeff %i %g \n", cnum, shapecoeff[cnum]);
			cnum++;
			
                }

		double beta = paramlist[otherdims+((MNStruct *)globalcontext)->numshapecoeff+2];

		for(int j =0; j < Nbins; j++){
	

			double timeinterval = 0.0001;
			double time = -1.0*double(Nbins)*timeinterval/2 + j*timeinterval;

			


			TNothpl(numcoeff,time/beta,Hermitepoly[j]);
			for(int k =0; k < numcoeff; k++){
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
				Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*time*time/beta/beta);
			}

		}

		shapecoeff[0] = 1.0;
		dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');

                for(int j = 0; j < Nbins; j++){
                        ProfileErr[j] += weight*(shapevec[j] - MeanProfile[j])*(shapevec[j] - MeanProfile[j]);
                }
        }


	for(int i = 0; i < Nbins; i++){
		ProfileErr[i] = sqrt(ProfileErr[i]/weightsum);
	}


	for(int i = 0; i < Nbins; i++){
		printf("%i %g %g \n", i, MeanProfile[i], ProfileErr[i]);
	}

	

	//profilefile.close();

}



double  WhiteLogLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	clock_t startClock,endClock;

	double *EFAC;
	double *EQUAD;
	int pcount=0;
	
	int numfit=((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;
	long double LDparams[numfit];
	double Fitparams[numfit];
	double *Resvec=new double[((MNStruct *)globalcontext)->pulse->nobs];

	double NLphase=0;
	//for(int p=0;p<ndim;p++){

	//	Cube[p]=(((MNStruct *)globalcontext)->Dpriors[p][1]-((MNStruct *)globalcontext)->Dpriors[p][0])*Cube[p]+((MNStruct *)globalcontext)->Dpriors[p][0];
	//}

	if(((MNStruct *)globalcontext)->doLinear==0){
	
		for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){
			LDparams[p]=Cube[p]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		}

		NLphase=(double)LDparams[0];
		pcount++;
		for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
			((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
			pcount++;
		}
		for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
			((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
			pcount++;
		}
		
//		fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
		formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);       /* Form residuals */
//		
//		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
//			Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+phase;
//		}
	
	}
	else if(((MNStruct *)globalcontext)->doLinear==1){
	
		for(int p=0;p < numfit; p++){
			Fitparams[p]=Cube[p];
			//printf("FitP: %i %g \n",p,Cube[p]);
			pcount++;
		}
		
		double *Fitvec=new double[((MNStruct *)globalcontext)->pulse->nobs];

		dgemv(((MNStruct *)globalcontext)->DMatrix,Fitparams,Fitvec,((MNStruct *)globalcontext)->pulse->nobs,numfit,'N');
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]=((MNStruct *)globalcontext)->pulse->obsn[o].residual-Fitvec[o];
			//printf("FitVec: %i %g \n",o,Fitvec[o]);
		}
		
		delete[] Fitvec;
	}

	
	double **Steps;
	if(((MNStruct *)globalcontext)->incStep > 0){
		
		Steps=new double*[((MNStruct *)globalcontext)->incStep];
		
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			Steps[i]=new double[2];
			Steps[i][0] = Cube[pcount];
			pcount++;
			Steps[i][1] = Cube[pcount];
			pcount++;
		}
	}
		

	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0, Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0, Cube[pcount]);
			pcount++;
		}
	}				

	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,2*Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,2*Cube[pcount]);
			pcount++;
                }
        }

        if(((MNStruct *)globalcontext)->doLinear==0){

                fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
                formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);       /* Form residuals */
                for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			  //printf("res: %i %g \n", o, (double)((MNStruct *)globalcontext)->pulse->obsn[o].residual);
                          Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+NLphase;
                }
        }


	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			double StepAmp = Steps[i][0];
			double StepTime = Steps[i][1];

			for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[o1].bat ;
				if( time > StepTime){
					Resvec[o1] += StepAmp;
				}
			}
		}
	}





	double Chisq=0;
	double noiseval=0;
	double detN=0;
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){

                if(((MNStruct *)globalcontext)->numFitEQUAD < 2 && ((MNStruct *)globalcontext)->numFitEFAC < 2){
			if(((MNStruct *)globalcontext)->whitemodel == 0){
				noiseval=pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6))*EFAC[0],2) + EQUAD[0];
			}
			else if(((MNStruct *)globalcontext)->whitemodel == 1){
				noiseval= EFAC[0]*EFAC[0]*(pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6)),2) + EQUAD[0]);
			}
                }
                else if(((MNStruct *)globalcontext)->numFitEQUAD > 1 || ((MNStruct *)globalcontext)->numFitEFAC > 1){
			if(((MNStruct *)globalcontext)->whitemodel == 0){
                       	 	noiseval=pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6))*EFAC[((MNStruct *)globalcontext)->sysFlags[o]],2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]];
			}
			else if(((MNStruct *)globalcontext)->whitemodel == 1){
                       	 	noiseval=EFAC[((MNStruct *)globalcontext)->sysFlags[o]]*EFAC[((MNStruct *)globalcontext)->sysFlags[o]]*(pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6)),2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]);
			}
                }
	
	

		Chisq += pow(Resvec[o],2)/noiseval;
		detN += log(noiseval);
	}
	
	double lnew = 0;
	if(isnan(detN) || isinf(detN) || isnan(Chisq) || isinf(Chisq)){

		lnew=-pow(10.0,200);
	}
	else{
		lnew = -0.5*(((MNStruct *)globalcontext)->pulse->nobs*log(2*M_PI) + detN + Chisq);	
	}

	delete[] EFAC;
	delete[] Resvec;
	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			delete[] Steps[i];
		}
		delete[] Steps;
	}

	return lnew;
}



void LRedLogLike(double *Cube, int &ndim, int &npars, double &lnew, void *globalcontext)
{

	clock_t startClock,endClock;
	double phase=0;
	double *EFAC;
	double *EQUAD;
	int pcount=0;
	int bad=0;
	int numfit=((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;
	long double LDparams[numfit];
	double Fitparams[numfit];
	double *Resvec=new double[((MNStruct *)globalcontext)->pulse->nobs];

	for(int p=0;p<ndim;p++){

		Cube[p]=(((MNStruct *)globalcontext)->Dpriors[p][1]-((MNStruct *)globalcontext)->Dpriors[p][0])*Cube[p]+((MNStruct *)globalcontext)->Dpriors[p][0];
	}

	if(((MNStruct *)globalcontext)->doLinear==0){
	
		for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){
			LDparams[p]=Cube[p]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		}

		phase=(double)LDparams[0];
		pcount++;
		for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
			((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
			pcount++;
		}
		for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
			((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
			pcount++;
		}
		
		if(((MNStruct *)globalcontext)->pulse->param[param_dmmodel].fitFlag[0] == 1){
			int DMnum=((MNStruct *)globalcontext)->pulse[0].dmoffsDMnum;
			for(int i =0; i < DMnum; i++){
				((MNStruct *)globalcontext)->pulse[0].dmoffsDM[i]=Cube[ndim-DMnum+i];
			}
		}
		
		
		fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
		formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);       /* Form residuals */
		
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+phase;
			
		}
// 		printf("Phase: %g \n", phase);
	
	}
	else if(((MNStruct *)globalcontext)->doLinear==1){
	
		for(int p=0;p < numfit; p++){
			Fitparams[p]=Cube[p];
			pcount++;
		}
		
		double *Fitvec=new double[((MNStruct *)globalcontext)->pulse->nobs];

		dgemv(((MNStruct *)globalcontext)->DMatrix,Fitparams,Fitvec,((MNStruct *)globalcontext)->pulse->nobs,numfit,'N');
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]=((MNStruct *)globalcontext)->pulse->obsn[o].residual-Fitvec[o];
		}
		
		delete[] Fitvec;
	}

	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			double StepAmp = Cube[pcount];
			pcount++;
			double StepTime = Cube[pcount];
			pcount++;
			for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
				double time = (double)((MNStruct *)globalcontext)->pulse->obsn[o1].bat;
				if(time > StepTime){
					Resvec[o1] += StepAmp;
				}
			}
		}
	}
		

	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=Cube[pcount];
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=Cube[pcount];
			pcount++;
		}
	}				

	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,2*Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,2*Cube[pcount]);
			pcount++;
                }
        }

	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int FitDMCoeff=2*(((MNStruct *)globalcontext)->numFitDMCoeff);
	if(((MNStruct *)globalcontext)->incFloatDM != 0)FitDMCoeff+=2*((MNStruct *)globalcontext)->incFloatDM;
	if(((MNStruct *)globalcontext)->incFloatRed != 0)FitRedCoeff+=2*((MNStruct *)globalcontext)->incFloatRed;
    	int totCoeff=0;
    	if(((MNStruct *)globalcontext)->incRED != 0)totCoeff+=FitRedCoeff;
    	if(((MNStruct *)globalcontext)->incDM != 0)totCoeff+=FitDMCoeff;

	double *WorkNoise=new double[((MNStruct *)globalcontext)->pulse->nobs];
	double *powercoeff=new double[totCoeff];

	double tdet=0;
	double timelike=0;
	double timelike2=0;

	if(((MNStruct *)globalcontext)->whitemodel == 0){
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			
			WorkNoise[o]=pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6))*EFAC[((MNStruct *)globalcontext)->sysFlags[o]],2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]];
			
			tdet=tdet+log(WorkNoise[o]);
			WorkNoise[o]=1.0/WorkNoise[o];
			timelike=timelike+pow(Resvec[o],2)*WorkNoise[o];
			timelike2=timelike2+pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].residual,2)*WorkNoise[o];

		}
	}
	else if(((MNStruct *)globalcontext)->whitemodel == 1){
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			
			WorkNoise[o]=EFAC[((MNStruct *)globalcontext)->sysFlags[o]]*EFAC[((MNStruct *)globalcontext)->sysFlags[o]]*(pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6)),2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]);
			
			tdet=tdet+log(WorkNoise[o]);
			WorkNoise[o]=1.0/WorkNoise[o];
			timelike=timelike+pow(Resvec[o],2)*WorkNoise[o];
			timelike2=timelike2+pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].residual,2)*WorkNoise[o];

		}
	}

	
	double *NFd = new double[totCoeff];
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
	}

	double **NF=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		NF[i]=new double[totCoeff];
	}

	double **FNF=new double*[totCoeff];
	for(int i=0;i<totCoeff;i++){
		FNF[i]=new double[totCoeff];
	}





	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }
// 	printf("Total time span = %.6f days = %.6f years\n",end-start,(end-start)/365.25);
	double maxtspan=end-start;

        double *freqs = new double[totCoeff];

        double *DMVec=new double[((MNStruct *)globalcontext)->pulse->nobs];
        double DMKappa = 2.410*pow(10.0,-16);
        int startpos=0;
        double freqdet=0;
        if(((MNStruct *)globalcontext)->incRED==2){
                for (int i=0; i<FitRedCoeff/2; i++){
                        int pnum=pcount;
                        double pc=Cube[pcount];
                        freqs[i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
                        freqs[i+FitRedCoeff/2]=freqs[i];

                        powercoeff[i]=pow(10.0,pc)/(maxtspan*24*60*60);///(365.25*24*60*60)/4;
                        powercoeff[i+FitRedCoeff/2]=powercoeff[i];
                        freqdet=freqdet+2*log(powercoeff[i]);
                        pcount++;
                }


                for(int i=0;i<FitRedCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);
                        }
                }

                for(int i=0;i<FitRedCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
                        }
                }



                startpos=FitRedCoeff;

        }
   else if(((MNStruct *)globalcontext)->incRED==3){

                double redamp=Cube[pcount];
                pcount++;
                double redindex=Cube[pcount];
                pcount++;
// 		printf("red: %g %g \n", redamp, redindex);
                 redamp=pow(10.0, redamp);

                freqdet=0;
                 for (int i=0; i<FitRedCoeff/2; i++){

                        freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
                        freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];

                        powercoeff[i]=redamp*redamp*pow((freqs[i]*365.25),-1.0*redindex)/(maxtspan*24*60*60);///(365.25*24*60*60)/4;
                        powercoeff[i+FitRedCoeff/2]=powercoeff[i];
                        freqdet=freqdet+2*log(powercoeff[i]);


                 }

                for(int i=0;i<FitRedCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);
                        }
                }

                for(int i=0;i<FitRedCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
                        }
                }


                startpos=FitRedCoeff;

        }


       if(((MNStruct *)globalcontext)->incDM==2){

                for (int i=0; i<FitDMCoeff/2; i++){
                        int pnum=pcount;
                        double pc=Cube[pcount];
                        freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[startpos+i]/maxtspan;
                        freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];

                        powercoeff[startpos+i]=pow(10.0,pc)/(maxtspan*24*60*60);///(365.25*24*60*60)/4;
                        powercoeff[startpos+i+FitDMCoeff/2]=powercoeff[startpos+i];
                        freqdet=freqdet+2*log(powercoeff[startpos+i]);
                        pcount++;
                }

                for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                        DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                }

                for(int i=0;i<FitDMCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                        }
                }

                for(int i=0;i<FitDMCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][startpos+i+FitDMCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                        }
                }



        }
        else if(((MNStruct *)globalcontext)->incDM==3){
                double DMamp=Cube[pcount];
                pcount++;
                double DMindex=Cube[pcount];
                pcount++;

                DMamp=pow(10.0, DMamp);

                 for (int i=0; i<FitDMCoeff/2; i++){
                        freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[startpos+i]/maxtspan;
                        freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];

                        powercoeff[startpos+i]=DMamp*DMamp*pow((freqs[startpos+i]*365.25),-1.0*DMindex)/(maxtspan*24*60*60);///(365.25*24*60*60)/4;
                        powercoeff[startpos+i+FitDMCoeff/2]=powercoeff[startpos+i];
                        freqdet=freqdet+2*log(powercoeff[startpos+i]);


                 }
                for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                        DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                }


                for(int i=0;i<FitDMCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                        }
                }

                for(int i=0;i<FitDMCoeff/2;i++){
                        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                                double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                                FMatrix[k][startpos+i+FitDMCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                        }
                }



        }





// 	makeFourier(((MNStruct *)globalcontext)->pulse, FitCoeff, FMatrix);

	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		for(int j=0;j<totCoeff;j++){
// 			printf("%i %i %g %g \n",i,j,WorkNoise[i],FMatrix[i][j]);
			NF[i][j]=WorkNoise[i]*FMatrix[i][j];
		}
	}
	dgemv(NF,Resvec,NFd,((MNStruct *)globalcontext)->pulse->nobs,totCoeff,'T');
	dgemm(FMatrix, NF , FNF, ((MNStruct *)globalcontext)->pulse->nobs, totCoeff, ((MNStruct *)globalcontext)->pulse->nobs, totCoeff, 'T', 'N');


	double **PPFM=new double*[totCoeff];
	for(int i=0;i<totCoeff;i++){
		PPFM[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			PPFM[i][j]=0;
		}
	}


	for(int c1=0; c1<totCoeff; c1++){
		PPFM[c1][c1]=1.0/powercoeff[c1];
	}



	for(int j=0;j<totCoeff;j++){
		for(int k=0;k<totCoeff;k++){
			PPFM[j][k]=PPFM[j][k]+FNF[j][k];
		}
	}

        double jointdet=0;
        double freqlike=0;
       double *WorkCoeff = new double[totCoeff];
       for(int o1=0;o1<totCoeff; o1++){
                WorkCoeff[o1]=NFd[o1];
        }


	    int info=0;
        dpotrfInfo(PPFM, totCoeff, jointdet,info);
        dpotrs(PPFM, WorkCoeff, totCoeff);
        for(int j=0;j<totCoeff;j++){
                freqlike += NFd[j]*WorkCoeff[j];
        }
	
	lnew=-0.5*(jointdet+tdet+freqdet+timelike-freqlike);

	if(isnan(lnew) || isinf(lnew)){

		lnew=-pow(10.0,200);
		
	}
	
	//printf("CPULike: %g %g %g %g %g %g \n", lnew, jointdet, tdet, freqdet, timelike, freqlike);
	delete[] DMVec;
	delete[] WorkCoeff;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] WorkNoise;
	delete[] powercoeff;
	delete[] Resvec;
	delete[] NFd;
	delete[] freqs;

	for (int j = 0; j < totCoeff; j++){
		delete[] PPFM[j];
	}
	delete[] PPFM;

	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[] NF[j];
	}
	delete[] NF;

	for (int j = 0; j < totCoeff; j++){
		delete[] FNF[j];
	}
	delete[] FNF;

	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[] FMatrix[j];
	}
	delete[] FMatrix;

}


double  FastNewLRedMarginLogLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){




	
	printf("In fast like\n");
	int pcount=0;
	
	int numfit=((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;
	long double LDparams[numfit];
	double Fitparams[numfit];
	double *Resvec=new double[((MNStruct *)globalcontext)->pulse->nobs];
	int fitcount=0;
	
	pcount=0;

	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){
		if(((MNStruct *)globalcontext)->Dpriors[p][1] != ((MNStruct *)globalcontext)->Dpriors[p][0]){

			double val = 0;
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 0){
				val = Cube[fitcount];
			}
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 1){
				val = pow(10.0,Cube[fitcount]);
			}
			LDparams[p]=val*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
			fitcount++;
		}
		else if(((MNStruct *)globalcontext)->Dpriors[p][1] == ((MNStruct *)globalcontext)->Dpriors[p][0]){
			LDparams[p]=((MNStruct *)globalcontext)->Dpriors[p][0]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		}


	}
	pcount=0;
	double phase=(double)LDparams[0];
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	
	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);       
	
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+phase;

	}

	
	pcount=fitcount;

	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			double StepAmp = Cube[pcount];
			pcount++;
			double StepTime = Cube[pcount];
			pcount++;
			for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
				if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > StepTime){
					Resvec[o1] += StepAmp;
				}
			}
		}
	}	


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract GLitches/////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	for(int i = 0 ; i < ((MNStruct *)globalcontext)->incGlitch; i++){
		double GlitchMJD = Cube[pcount];
		pcount++;

		double *GlitchAmps = new double[2];
		if(((MNStruct *)globalcontext)->incGlitchTerms==1 || ((MNStruct *)globalcontext)->incGlitchTerms==3){
			GlitchAmps[0]  = Cube[pcount];
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
			GlitchAmps[0]  = Cube[pcount];
                        pcount++;
			GlitchAmps[1]  = Cube[pcount];
                        pcount++;
		}


		for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
                        if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > GlitchMJD){

				if(((MNStruct *)globalcontext)->incGlitchTerms==1){

					long double arg=0;
					arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;

				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
					for(int j = 0; j < ((MNStruct *)globalcontext)->incGlitchTerms; j++){

						long double arg=0;
						if(j==0){
							arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
						}
						if(j==1){
							arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
						}
						double darg = (double)arg;
						Resvec[o1] += GlitchAmps[j]*darg;
					}
				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==3){
					long double arg=0;
					arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;
				}


			}
		}
	}
	
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get White Noise vector///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  




	double *Noise;	
	Noise=new double[((MNStruct *)globalcontext)->pulse->nobs];
	
	
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		Noise[o] = ((MNStruct *)globalcontext)->PreviousNoise[o];
	}
		
	

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Time domain likelihood//////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


/* 
	static unsigned int oldcw;
	if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
		fpu_fix_start(&oldcw);
	}
*/	


	double tdet=0;
	double timelike=0;

	for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		timelike+=Resvec[o]*Resvec[o]*Noise[o];
		tdet -= log(Noise[o]);
	}

/*
	dd_real ddtimelike=0.0;
	if(((MNStruct *)globalcontext)->uselongdouble ==1){
		for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			dd_real res = (dd_real)Resvec[o];
			dd_real Nval = (dd_real)Noise[o];
			dd_real chicomp = res*res*Nval;
			ddtimelike += chicomp;
		}
	}

	
        qd_real qdtimelike=0.0;
        if(((MNStruct *)globalcontext)->uselongdouble ==2){
                for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){

                        qd_real res = (qd_real)Resvec[o];
                        qd_real Nval = (qd_real)Noise[o];
                        qd_real chicomp = res*res*Nval;

                        qdtimelike+=chicomp;
                }
        }

*/
	printf("Fast time like %g %g %i\n", timelike, tdet, ((MNStruct *)globalcontext)->totCoeff);

//////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////Do Algebra/////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


        int totCoeff=((MNStruct *)globalcontext)->totCoeff;

        int TimetoMargin=0;
        for(int i =0; i < ((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps; i++){
                if(((MNStruct *)globalcontext)->LDpriors[i][2]==1)TimetoMargin++;
        }
	
	


	int totalsize=totCoeff + TimetoMargin;

	double *NTd = new double[totalsize];

	double **NT=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		NT[i]=new double[totalsize];
		for(int j=0;j<totalsize;j++){
                        NT[i][j]=((MNStruct *)globalcontext)->PreviousNT[i][j];
                }

	}

	double **TNT=new double*[totalsize];
	for(int i=0;i<totalsize;i++){
		TNT[i]=new double[totalsize];
		for(int j=0;j<totalsize;j++){
			TNT[i][j] = ((MNStruct *)globalcontext)->PreviousTNT[i][j];
		}
	}
	

	



	dgemv(NT,Resvec,NTd,((MNStruct *)globalcontext)->pulse->nobs,totalsize,'T');



	double freqlike=0;
	double *WorkCoeff = new double[totalsize];
	for(int o1=0;o1<totalsize; o1++){
	    WorkCoeff[o1]=NTd[o1];
	}

	int globalinfo=0;
	int info=0;
	double jointdet = ((MNStruct *)globalcontext)->PreviousJointDet;	
	double freqdet = ((MNStruct *)globalcontext)->PreviousFreqDet;
	double uniformpriorterm = ((MNStruct *)globalcontext)->PreviousUniformPrior;

	info=0;
	dpotrsInfo(TNT, WorkCoeff, totalsize, info);
	if(info != 0)globalinfo=1;


	for(int j=0;j<totalsize;j++){
	    freqlike += NTd[j]*WorkCoeff[j];

	}


	double lnew = -0.5*(tdet+jointdet+freqdet+timelike-freqlike) + uniformpriorterm;
	
	if(isnan(lnew) || isinf(lnew) || globalinfo != 0){

		lnew=-pow(10.0,20);
		
	}

/*
        if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
                fpu_fix_end(&oldcw);
        }
*/


	delete[] WorkCoeff;
	delete[] NTd;
	delete[] Noise;
	delete[] Resvec;
	
	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[]NT[j];
	}
	delete[]NT;

	for (int j = 0; j < totalsize; j++){
		delete[]TNT[j];
	}
	delete[]TNT;

	return lnew;


}


void LRedLikeMNWrap(double *Cube, int &ndim, int &npars, double &lnew, void *context){



        for(int p=0;p<ndim;p++){

                Cube[p]=(((MNStruct *)globalcontext)->PriorsArray[p+ndim]-((MNStruct *)globalcontext)->PriorsArray[p])*Cube[p]+((MNStruct *)globalcontext)->PriorsArray[p];
        }


	double *DerivedParams = new double[npars];

	double result = NewLRedMarginLogLike(ndim, Cube, npars, DerivedParams, context);


	delete[] DerivedParams;

	lnew = result;

}

double  NewLRedMarginLogLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	double uniformpriorterm=0;
	clock_t startClock,endClock;

	double **EFAC;
	double *EQUAD;
	int pcount=0;
	
	int numfit=((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;
	long double LDparams[numfit];
	double Fitparams[numfit];
	double *Resvec=new double[((MNStruct *)globalcontext)->pulse->nobs];
	int fitcount=0;
	

	if(((MNStruct *)globalcontext)->doGrades == 1){
		int slowlike = 0;
		double myeps = pow(10.0, -10);
		for(int i = 0; i < ndim; i ++){
			
			int slowindex = ((MNStruct *)globalcontext)->hypercube_indices[i]-1;


			 printf("Cube: %i %i %i %g %g %g \n", i, slowindex, ((MNStruct *)globalcontext)->PolyChordGrades[slowindex], Cube[i], ((MNStruct *)globalcontext)->LastParams[i], fabs(Cube[i] - ((MNStruct *)globalcontext)->LastParams[i]));

			if(((MNStruct *)globalcontext)->PolyChordGrades[slowindex] == 1){

//				printf("Slow Cube: %i %g %g %g \n", i, Cube[slowindex], ((MNStruct *)globalcontext)->LastParams[slowindex], fabs(Cube[slowindex] - ((MNStruct *)globalcontext)->LastParams[slowindex]));

				if(fabs(Cube[slowindex] - ((MNStruct *)globalcontext)->LastParams[slowindex]) > myeps){
					slowlike = 1;
				}
			}

			((MNStruct *)globalcontext)->LastParams[i] = Cube[i];
		}

		if(slowlike == 0){

			double lnew = 0;
			if(((MNStruct *)globalcontext)->PreviousInfo == 0){
				lnew = FastNewLRedMarginLogLike(ndim, Cube, npars, DerivedParams, context);
			}
			else if(((MNStruct *)globalcontext)->PreviousInfo == 1){
				lnew = -pow(10.0,20);
			}

			return lnew;
		}
	}
				

	pcount=0;

	///////////////hacky glitch thing for Vela, comment this out for *anything else*/////////////////////////
	for(int p=10;p<18; p++){
//		Cube[p] = Cube[9];
	}
        for(int p=19;p<26; p++){
  //              Cube[p] = Cube[18];
        }

	///////////////end of hacky glitch thing for Vela///////////////////////////////////////////////////////
	
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){
		if(((MNStruct *)globalcontext)->Dpriors[p][1] != ((MNStruct *)globalcontext)->Dpriors[p][0]){

			double val = 0;
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 0){
				val = Cube[fitcount];
			}
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 1){
				val = pow(10.0,Cube[fitcount]);
			}
                        if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 2){
                                val = pow(10.0,Cube[fitcount]);
				uniformpriorterm += log(val);
                        }
			LDparams[p]=val*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
			fitcount++;
		}
		else if(((MNStruct *)globalcontext)->Dpriors[p][1] == ((MNStruct *)globalcontext)->Dpriors[p][0]){
			LDparams[p]=((MNStruct *)globalcontext)->Dpriors[p][0]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		}


	}
	pcount=0;
	double phase=(double)LDparams[0];
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	if(((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps > 0){	
		fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
		formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);       /* Form residuals */
	}
	
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+phase;

	}

	
	pcount=fitcount;

	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			double StepAmp = Cube[pcount];
			pcount++;
			double StepTime = Cube[pcount];
			pcount++;
			for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
				if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > StepTime){
					Resvec[o1] += StepAmp;
				}
			}
		}
	}	


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract GLitches/////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	for(int i = 0 ; i < ((MNStruct *)globalcontext)->incGlitch; i++){
		double GlitchMJD = Cube[pcount];
		pcount++;

		double *GlitchAmps = new double[2];
		if(((MNStruct *)globalcontext)->incGlitchTerms==1 || ((MNStruct *)globalcontext)->incGlitchTerms==3){
			GlitchAmps[0]  = Cube[pcount];
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
			GlitchAmps[0]  = Cube[pcount];
                        pcount++;
			GlitchAmps[1]  = Cube[pcount];
                        pcount++;
		}


		for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
                        if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > GlitchMJD){

				if(((MNStruct *)globalcontext)->incGlitchTerms==1){

					long double arg=0;
					arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;

				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
					for(int j = 0; j < ((MNStruct *)globalcontext)->incGlitchTerms; j++){

						long double arg=0;
						if(j==0){
							arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
						}
						if(j==1){
							arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
						}
						double darg = (double)arg;
						Resvec[o1] += GlitchAmps[j]*darg;
					}
				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==3){
					long double arg=0;
					arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;
				}


			}
		}
	}
	
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get White Noise vector///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
					EFAC[n-1][o]=1;
				}
			}
			else{
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                   EFAC[n-1][o]=0;
                }
			}
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
					EFAC[n-1][o]=pow(10.0,Cube[pcount]);
					if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[n-1][o]);}
				}
				pcount++;
			}
			else{
                                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){

                                        EFAC[n-1][o]=pow(10.0,Cube[pcount]);
                                }
                                pcount++;
                        }
		}
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
					EFAC[n-1][p]=pow(10.0,Cube[pcount]);
					if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[n-1][p]);}
					pcount++;
				}
			}
			else{
                                for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
                                        EFAC[n-1][p]=pow(10.0,Cube[pcount]);
                                        pcount++;
                                }
                        }
		}
	}	

		
	//printf("Equad %i \n", ((MNStruct *)globalcontext)->numFitEQUAD);
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,2*Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount]));}
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){

			if(((MNStruct *)globalcontext)->includeEQsys[o] == 1){
				//printf("Cube: %i %i %g \n", o, pcount, Cube[pcount]);
				EQUAD[o]=pow(10.0,2*Cube[pcount]);
				if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount])); }
				pcount++;
			}
			else{
				EQUAD[o]=0;
			}
			//printf("Equad? %i %g \n", o, EQUAD[o]);
		}
    	}
    

        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,2*Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount]));}
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
            SQUAD[o]=pow(10.0,2*Cube[pcount]);
	    if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount])); }

			pcount++;
        }
    }
 
	double *ECORRPrior;
	if(((MNStruct *)globalcontext)->incNGJitter >0){
		double *ECorrCoeffs=new double[((MNStruct *)globalcontext)->incNGJitter];	
		for(int i =0; i < ((MNStruct *)globalcontext)->incNGJitter; i++){
			ECorrCoeffs[i] = pow(10.0, 2*Cube[pcount]);
			pcount++;
		}
    		ECORRPrior = new double[((MNStruct *)globalcontext)->numNGJitterEpochs];
		for(int i =0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
			ECORRPrior[i] = ECorrCoeffs[((MNStruct *)globalcontext)->NGJitterSysFlags[i]];
		}

		delete[] ECorrCoeffs;
	} 



	double SolarWind=0;
	double WhiteSolarWind = 0;

	if(((MNStruct *)globalcontext)->FitSolarWind == 1){
		SolarWind = Cube[pcount];
		pcount++;

		//printf("Solar Wind: %g \n", SolarWind);
	}
	if(((MNStruct *)globalcontext)->FitWhiteSolarWind == 1){
		WhiteSolarWind = pow(10.0, Cube[pcount]);
		pcount++;

		//printf("White Solar Wind: %g \n", WhiteSolarWind);
	}



	double *Noise;	
	double *BATvec;
	Noise=new double[((MNStruct *)globalcontext)->pulse->nobs];
	BATvec=new double[((MNStruct *)globalcontext)->pulse->nobs];
	
	
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		BATvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat;
	}
		
		
	if(((MNStruct *)globalcontext)->whitemodel == 0){
	
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			double EFACterm=0;
			double noiseval=0;
			double ShannonJitterTerm=0;

			double SWTerm = WhiteSolarWind*((MNStruct *)globalcontext)->pulse->obsn[o].tdis2/((MNStruct *)globalcontext)->pulse->ne_sw;		
						
			
			if(((MNStruct *)globalcontext)->useOriginalErrors==0){
				noiseval=((MNStruct *)globalcontext)->pulse->obsn[o].toaErr;
			}
			else if(((MNStruct *)globalcontext)->useOriginalErrors==1){
				noiseval=((MNStruct *)globalcontext)->pulse->obsn[o].origErr;
			}


			for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
				EFACterm=EFACterm + pow((noiseval*pow(10.0,-6))/pow(pow(10.0,-7),n-1),n)*EFAC[n-1][((MNStruct *)globalcontext)->sysFlags[o]];
			}	
			
			if(((MNStruct *)globalcontext)->incShannonJitter > 0){	
			 	ShannonJitterTerm=SQUAD[((MNStruct *)globalcontext)->sysFlags[o]]*((MNStruct *)globalcontext)->TobsInfo[o]/1000.0;
			}

			Noise[o]= 1.0/(pow(EFACterm,2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]+ShannonJitterTerm + SWTerm*SWTerm);

		}
		
	}
	else if(((MNStruct *)globalcontext)->whitemodel == 1){
	
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){

			Noise[o]=1.0/(EFAC[0][((MNStruct *)globalcontext)->sysFlags[o]]*EFAC[0][((MNStruct *)globalcontext)->sysFlags[o]]*(pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6)),2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]));
		}
		
	}

	
	

		
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form the Design Matrix////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////  

	int TimetoMargin=0;
	for(int i =0; i < ((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps; i++){
		if(((MNStruct *)globalcontext)->LDpriors[i][2]==1)TimetoMargin++;
	}

	double *TNDM;

	if(TimetoMargin != ((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps){

/*
		TNDM=new double*[((MNStruct *)globalcontext)->pulse->nobs]; 
		for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
			TNDM[i]=new double[TimetoMargin];
		}

*/


		//printf("allocating: %i \n", ((MNStruct *)globalcontext)->pulse->nobs*TimetoMargin);
		TNDM=new double[((MNStruct *)globalcontext)->pulse->nobs*TimetoMargin];
		//printf("getting d matrix %i \n", TimetoMargin);
		getCustomDVectorLike(globalcontext, TNDM);
		//printf("got d matrix \n");
		
//		getCustomDMatrixLike(globalcontext, TNDM);

//		double* S = new double[TimetoMargin];

/*
		double** U = new double*[((MNStruct *)globalcontext)->pulse->nobs];
		for(int k=0; k < ((MNStruct *)globalcontext)->pulse->nobs; k++){
			U[k] = new double[((MNStruct *)globalcontext)->pulse->nobs];


		}

*/
//		double** VT = new double*[TimetoMargin]; 
//		for (int k=0; k<TimetoMargin; k++) VT[k] = new double[TimetoMargin];

//		double* U = new double[((MNStruct *)globalcontext)->pulse->nobs*TimetoMargin];
		//printf("doing svd \n");
		vector_dgesvd(TNDM,((MNStruct *)globalcontext)->pulse->nobs, TimetoMargin);
		//printf("done svd\n");
//		dgesvd(TNDM,((MNStruct *)globalcontext)->pulse->nobs, TimetoMargin, S, U, VT);
//		delete[]S;	

//		for (int j = 0; j < TimetoMargin; j++){
//			delete[]VT[j];
//		}
	
//		delete[]VT;
	
		
/*
		for(int j=0;j<((MNStruct *)globalcontext)->pulse->nobs;j++){
			for(int k=0;k < TimetoMargin;k++){
					//TNDM[j][k]=U[j][k];
					TNDM[j][k]=U[j*((MNStruct *)globalcontext)->pulse->nobs + k];
			}
		}
*/
//		for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
//			delete[]U[j];
//		}
//		delete[]U;
		
	}
	else{	
		TNDM=new double[((MNStruct *)globalcontext)->pulse->nobs*TimetoMargin];

		for(int j=0;j<((MNStruct *)globalcontext)->pulse->nobs;j++){
			for(int k=0;k < TimetoMargin;k++){
					TNDM[j + k*((MNStruct *)globalcontext)->pulse->nobs]=((MNStruct *)globalcontext)->DMatrix[j][k];
			}
		}
		//TNDM = ((MNStruct *)globalcontext)->DMatrix;
	}


//////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////////////Set up Coefficients///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////  



	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }

	double maxtspan=1*(end-start);
	double averageTSamp=2*maxtspan/((MNStruct *)globalcontext)->pulse->nobs;

	double **DMEventInfo;

	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int FitDMCoeff=2*(((MNStruct *)globalcontext)->numFitDMCoeff);
	int FitDMScatterCoeff=2*(((MNStruct *)globalcontext)->numFitDMScatterCoeff);
	int FitGroupNoiseCoeff = 2*((MNStruct *)globalcontext)->numFitGroupNoiseCoeff;

	if(((MNStruct *)globalcontext)->incFloatDM != 0)FitDMCoeff+=2*((MNStruct *)globalcontext)->incFloatDM;
	if(((MNStruct *)globalcontext)->incFloatRed != 0)FitRedCoeff+=2*((MNStruct *)globalcontext)->incFloatRed;
	int DMEventCoeff=0;
	if(((MNStruct *)globalcontext)->incDMEvent != 0){
		DMEventInfo=new double*[((MNStruct *)globalcontext)->incDMEvent];
		for(int i =0; i < ((MNStruct *)globalcontext)->incDMEvent; i++){
			DMEventInfo[i]=new double[7];
			DMEventInfo[i][0]=Cube[pcount]; //Start time
			pcount++;
			DMEventInfo[i][1]=Cube[pcount]; //Length
			pcount++;
			DMEventInfo[i][2]=pow(10.0, Cube[pcount]); //Amplitude
			pcount++;
			DMEventInfo[i][3]=Cube[pcount]; //SpectralIndex
			pcount++;
			DMEventInfo[i][4]=Cube[pcount]; //Offset
			pcount++;
			DMEventInfo[i][5]=Cube[pcount]; //Linear
			pcount++;
			DMEventInfo[i][6]=Cube[pcount]; //Quadratic
			pcount++;
			DMEventCoeff+=2*int(DMEventInfo[i][1]/averageTSamp);

			}
	}


	int totCoeff=0;
	if(((MNStruct *)globalcontext)->incRED != 0 || ((MNStruct *)globalcontext)->incGWB == 1)totCoeff+=FitRedCoeff;
	if(((MNStruct *)globalcontext)->incDM != 0)totCoeff+=FitDMCoeff;

	if(((MNStruct *)globalcontext)->incDMScatter == 1 || ((MNStruct *)globalcontext)->incDMScatter == 2  || ((MNStruct *)globalcontext)->incDMScatter == 3)totCoeff+=FitDMScatterCoeff;
	if(((MNStruct *)globalcontext)->incDMScatter == 4 ||((MNStruct *)globalcontext)->incDMScatter == 5 )totCoeff+=3*FitDMScatterCoeff;

	if(((MNStruct *)globalcontext)->incDMEvent != 0)totCoeff+=DMEventCoeff; 
	if(((MNStruct *)globalcontext)->incNGJitter >0)totCoeff+=((MNStruct *)globalcontext)->numNGJitterEpochs;

	if(((MNStruct *)globalcontext)->incGroupNoise > 0)totCoeff += ((MNStruct *)globalcontext)->incGroupNoise*FitGroupNoiseCoeff;





	double *powercoeff=new double[totCoeff];
	for(int o=0;o<totCoeff; o++){
		powercoeff[o]=0;
	}
	
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			FMatrix[i][j]=0;
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Red Noise///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


    double *freqs = new double[totCoeff];

    double *DMVec=new double[((MNStruct *)globalcontext)->pulse->nobs];
    double DMKappa = 2.410*pow(10.0,-16);
    int startpos=0;
    double freqdet=0;
    double GWBAmpPrior=0;
    int badAmp=0;

	if(((MNStruct *)globalcontext)->incRED > 0 || ((MNStruct *)globalcontext)->incGWB == 1){


		if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 1){
			double fLow = pow(10.0, Cube[pcount]);
			pcount++;

			double deltaLogF = 0.1;
			double RedMidFreq = 2.0;

			double RedLogDiff = log10(RedMidFreq) - log10(fLow);
			int LogLowFreqs = floor(RedLogDiff/deltaLogF);

			double RedLogSampledDiff = LogLowFreqs*deltaLogF;
			double sampledFLow = floor(log10(fLow)/deltaLogF)*deltaLogF;
			
			int freqStartpoint = 0;


			for(int i =0; i < LogLowFreqs; i++){
				((MNStruct *)globalcontext)->sampleFreq[freqStartpoint]=pow(10.0, sampledFLow + i*RedLogSampledDiff/LogLowFreqs);
				freqStartpoint++;

			}

			for(int i =0;i < FitRedCoeff/2-LogLowFreqs; i++){
				((MNStruct *)globalcontext)->sampleFreq[freqStartpoint]=i+RedMidFreq;
				freqStartpoint++;
			}

		}

                if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 2){
                        double fLow = pow(10.0, Cube[pcount]);
                        pcount++;

                        for(int i =0;i < FitRedCoeff/2; i++){
                                ((MNStruct *)globalcontext)->sampleFreq[i]=((double)(i+1))*fLow;
                        }


                }

		for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
			
			freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
			freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];

		}

	}


	if(((MNStruct *)globalcontext)->incRED==2){

    
		for (int i=0; i<FitRedCoeff/2; i++){
			int pnum=pcount;
			double pc=Cube[pcount];

			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm += log(pow(10.0,pc)); }

			powercoeff[i]=pow(10.0,2*pc);
			powercoeff[i+FitRedCoeff/2]=powercoeff[i];
			pcount++;
		}
		
		
	            
	    startpos=FitRedCoeff;

	}
	else if(((MNStruct *)globalcontext)->incRED==3 || ((MNStruct *)globalcontext)->incRED==4){

		
		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitRedPL; pl ++){

			double Tspan = maxtspan;
                        double f1yr = 1.0/3.16e7;

			if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 2){
				Tspan=Tspan/((MNStruct *)globalcontext)->sampleFreq[0];
			}



			double redamp=Cube[pcount];
			pcount++;
			double redindex=Cube[pcount];
			pcount++;

			
			double cornerfreq=0;
			if(((MNStruct *)globalcontext)->incRED==4){
				cornerfreq=pow(10.0, Cube[pcount])/Tspan;
				pcount++;
			}

	
			
			redamp=pow(10.0, redamp);
			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm +=log(redamp); }



			double Agw=redamp;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				
				
				double rho=0;
				if(((MNStruct *)globalcontext)->incRED==3){	
 					rho = (Agw*Agw/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-redindex))/(Tspan*24*60*60);
				}
				if(((MNStruct *)globalcontext)->incRED==4){
					
			rho = pow((1+(pow((1.0/365.25)/cornerfreq,redindex/2))),2)*(Agw*Agw/12.0/(M_PI*M_PI))/pow((1+(pow(freqs[i]/cornerfreq,redindex/2))),2)/(Tspan*24*60*60)*pow(f1yr,-3.0);
				}
				//if(rho > pow(10.0,15))rho=pow(10.0,15);
 				powercoeff[i]+= rho;
 				powercoeff[i+FitRedCoeff/2]+= rho;

				

			}
		}
		


		int coefftovary=0;
		double amptovary=0.0;
		if(((MNStruct *)globalcontext)->varyRedCoeff==1){
			coefftovary=int(pow(10.0,Cube[pcount]))-1;
			pcount++;
			amptovary=pow(10.0,Cube[pcount]);
			pcount++;

			powercoeff[coefftovary]=amptovary;
			powercoeff[coefftovary+FitRedCoeff/2]=amptovary;	
		}		
		

		

		startpos=FitRedCoeff;

    }


	if(((MNStruct *)globalcontext)->incGWB==1){
		double GWBAmp=pow(10.0,Cube[pcount]);
		pcount++;
		GWBAmpPrior=log(GWBAmp);
		uniformpriorterm +=GWBAmpPrior;
		double Tspan = maxtspan;

		if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 2){
			Tspan=Tspan/((MNStruct *)globalcontext)->sampleFreq[0];
		}

		double f1yr = 1.0/3.16e7;
		for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
			double rho = (GWBAmp*GWBAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-4.333))/(Tspan*24*60*60);
			powercoeff[i]+= rho;
			powercoeff[i+FitRedCoeff/2]+= rho;
			//printf("%i %g %g \n", i, freqs[i], powercoeff[i]);
		}

		startpos=FitRedCoeff;
	}

	for (int i=0; i<FitRedCoeff/2; i++){
		freqdet=freqdet+2*log(powercoeff[i]);
	}


        for(int i=0;i<FitRedCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);
			FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);


                }
        }

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  Red Shape Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	int MarginRedShapelets = ((MNStruct *)globalcontext)->MarginRedShapeCoeff;
	int totalredshapecoeff = 0;
	int badshape = 0;

	double **RedShapeletMatrix;

	double globalwidth=0;
	if(((MNStruct *)globalcontext)->incRedShapeEvent != 0){


		if(MarginRedShapelets == 1){

			badshape = 1;

			totalredshapecoeff = ((MNStruct *)globalcontext)->numRedShapeCoeff*((MNStruct *)globalcontext)->incRedShapeEvent;

			

			RedShapeletMatrix = new double*[((MNStruct *)globalcontext)->pulse->nobs];
			for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
				RedShapeletMatrix[i] = new double[totalredshapecoeff];
					for(int j = 0; j < totalredshapecoeff; j++){
						RedShapeletMatrix[i][j] = 0;
					}
				}
			}


		        for(int i =0; i < ((MNStruct *)globalcontext)->incRedShapeEvent; i++){

				int numRedShapeCoeff=((MNStruct *)globalcontext)->numRedShapeCoeff;

		                double EventPos=Cube[pcount];
				pcount++;
		                double EventWidth=Cube[pcount];
				pcount++;

				globalwidth=EventWidth;



				double *RedshapeVec=new double[numRedShapeCoeff];

				double *RedshapeNorm=new double[numRedShapeCoeff];
				for(int c=0; c < numRedShapeCoeff; c++){
					RedshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
				}

				for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

					double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
					TNothpl(numRedShapeCoeff,HVal,RedshapeVec);

					for(int c=0; c < numRedShapeCoeff; c++){
						RedShapeletMatrix[k][i*numRedShapeCoeff+c] = RedshapeNorm[c]*RedshapeVec[c]*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
						if(RedShapeletMatrix[k][i*numRedShapeCoeff+c] > 0.01){ badshape = 0;}
						

					}

	
				}


				delete[] RedshapeVec;
				delete[] RedshapeNorm;

			}

		if(MarginRedShapelets == 0){

		        for(int i =0; i < ((MNStruct *)globalcontext)->incRedShapeEvent; i++){

				int numRedShapeCoeff=((MNStruct *)globalcontext)->numRedShapeCoeff;

		                double EventPos=Cube[pcount];
				pcount++;
		                double EventWidth=Cube[pcount];
				pcount++;

				globalwidth=EventWidth;


				double *Redshapecoeff=new double[numRedShapeCoeff];
				double *RedshapeVec=new double[numRedShapeCoeff];
				for(int c=0; c < numRedShapeCoeff; c++){
					Redshapecoeff[c]=Cube[pcount];
					pcount++;
				}


				double *RedshapeNorm=new double[numRedShapeCoeff];
				for(int c=0; c < numRedShapeCoeff; c++){
					RedshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
				}

				for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
					double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

					double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
					TNothpl(numRedShapeCoeff,HVal,RedshapeVec);
					double Redsignal=0;
					for(int c=0; c < numRedShapeCoeff; c++){
						Redsignal += RedshapeNorm[c]*RedshapeVec[c]*Redshapecoeff[c];
					}

					  Resvec[k] -= Redsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));

					  //printf("Shape Sig: %i %.10Lg %g \n", k, ((MNStruct *)globalcontext)->pulse->obsn[k].bat, Redsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2)));
	
				}




			delete[] Redshapecoeff;
			delete[] RedshapeVec;
			delete[] RedshapeNorm;

			}
		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract Sine Wave///////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	if(((MNStruct *)globalcontext)->incsinusoid == 1){
		double sineamp=pow(10.0,Cube[pcount]);
		pcount++;
		double sinephase=Cube[pcount];
		pcount++;
		double sinefreq=pow(10.0,Cube[pcount])/maxtspan;
		pcount++;		
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]-= sineamp*sin(2*M_PI*sinefreq*(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat + sinephase);
		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////DM Variations////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


       if(((MNStruct *)globalcontext)->incDM==2){

			for (int i=0; i<FitDMCoeff/2; i++){
				int pnum=pcount;
				double pc=Cube[pcount];
				freqs[startpos+i]=((MNStruct *)globalcontext)->sampleFreq[startpos/2 - ((MNStruct *)globalcontext)->incFloatRed+i]/maxtspan;
				freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];
	
				powercoeff[startpos+i]=pow(10.0,2*pc);
				powercoeff[startpos+i+FitDMCoeff/2]=powercoeff[startpos+i];
				freqdet=freqdet+2*log(powercoeff[startpos+i]);
				pcount++;
			}
           	 


                for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                        DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                }
                
            for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

        for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i+FitDMCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

        }
        else if(((MNStruct *)globalcontext)->incDM==3){

		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitDMPL; pl ++){
			double DMamp=Cube[pcount];
			pcount++;
			double DMindex=Cube[pcount];
			pcount++;

			
   			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
    			

			DMamp=pow(10.0, DMamp);
			if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(DMamp); }
			for (int i=0; i<FitDMCoeff/2; i++){
	
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[startpos/2 - ((MNStruct *)globalcontext)->incFloatRed +i]/maxtspan;
				freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];
				
 				double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-DMindex))/(maxtspan*24*60*60);	
 				powercoeff[startpos+i]+=rho;
 				powercoeff[startpos+i+FitDMCoeff/2]+=rho;
			}
		}
		
		
		int coefftovary=0;
		double amptovary=0.0;
		if(((MNStruct *)globalcontext)->varyDMCoeff==1){
			coefftovary=int(pow(10.0,Cube[pcount]))-1;
			pcount++;
			amptovary=pow(10.0,Cube[pcount])/(maxtspan*24*60*60);
			pcount++;

			powercoeff[startpos+coefftovary]=amptovary;
			powercoeff[startpos+coefftovary+FitDMCoeff/2]=amptovary;	
		}	
			
		
		for (int i=0; i<FitDMCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
        }

        for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

        for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i+FitDMCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

	startpos+=FitDMCoeff;

    }
    

    


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////DM Power Spectrum Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	
	if(((MNStruct *)globalcontext)->incDMEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMEvent; i++){
                        double DMamp=DMEventInfo[i][2];
                        double DMindex=DMEventInfo[i][3];
			double DMOff=DMEventInfo[i][4];
			double DMLinear=0;//DMEventInfo[i][5];
			double DMQuad=0;//DMEventInfo[i][6];

                        double Tspan = DMEventInfo[i][1];
                        double f1yr = 1.0/3.16e7;
			int DMEventnumCoeff=int(Tspan/averageTSamp);

                        for (int c=0; c<DMEventnumCoeff; c++){
                                freqs[startpos+c]=((double)(c+1))/Tspan;
                                freqs[startpos+c+DMEventnumCoeff]=freqs[startpos+c];

                                double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freqs[startpos+c]*365.25,(-DMindex))/(maxtspan*24*60*60);
                                powercoeff[startpos+c]+=rho;
                                powercoeff[startpos+c+DMEventnumCoeff]+=rho;
				freqdet=freqdet+2*log(powercoeff[startpos+c]);
                        }


			double *DMshapecoeff=new double[3];
			double *DMshapeVec=new double[3];
			DMshapecoeff[0]=DMOff;
			DMshapecoeff[1]=DMLinear;			
			DMshapecoeff[2]=DMQuad;



			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;


				if(time < DMEventInfo[i][0]+Tspan && time > DMEventInfo[i][0]){

					Resvec[k] -= DMOff*DMVec[k];
					Resvec[k] -= (time-DMEventInfo[i][0])*DMLinear*DMVec[k];
					Resvec[k] -= pow((time-DMEventInfo[i][0]),2)*DMQuad*DMVec[k];

				}
			}

		 	for(int c=0;c<DMEventnumCoeff;c++){
                		for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
					if(time < DMEventInfo[i][0]+Tspan && time > DMEventInfo[i][0]){
						FMatrix[k][startpos+c]=cos(2*M_PI*freqs[startpos+c]*time)*DMVec[k];
                        			FMatrix[k][startpos+c+DMEventnumCoeff]=sin(2*M_PI*freqs[startpos+c]*time)*DMVec[k];
					}
					else{
						FMatrix[k][startpos+c]=0;
						FMatrix[k][startpos+c+DMEventnumCoeff]=0;
					}
                		}
		        }

			startpos+=2*DMEventnumCoeff;



		}
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  DM Shape Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	if(((MNStruct *)globalcontext)->incDMShapeEvent != 0){

		if(((MNStruct *)globalcontext)->incDM == 0){
			        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                        		DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                		}
		}
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMShapeEvent; i++){

			int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=Cube[pcount];
			pcount++;


			globalwidth=EventWidth;

			double *DMshapecoeff=new double[numDMShapeCoeff];
			double *DMshapeVec=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *DMshapeNorm=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
				double DMsignal=0;
				for(int c=0; c < numDMShapeCoeff; c++){
					DMsignal += DMshapeNorm[c]*DMshapeVec[c]*DMshapecoeff[c]*DMVec[k];
				}

				  Resvec[k] -= DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] DMshapecoeff;
		delete[] DMshapeVec;
		delete[] DMshapeNorm;

		}
	}





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  DM Scatter Shape Events///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	if(((MNStruct *)globalcontext)->incDMScatterShapeEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMScatterShapeEvent; i++){

			int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMScatterShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=globalwidth;//Cube[pcount];
			pcount++;
                        double EventFScale=Cube[pcount];
			pcount++;

			//EventWidth=globalwidth;

			double *DMshapecoeff=new double[numDMShapeCoeff];
			double *DMshapeVec=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *DMshapeNorm=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
				double DMsignal=0;
				for(int c=0; c < numDMShapeCoeff; c++){
					DMsignal += DMshapeNorm[c]*DMshapeVec[c]*DMshapecoeff[c]*pow(DMVec[k],EventFScale/2.0);
				}

				  Resvec[k] -= DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] DMshapecoeff;
		delete[] DMshapeVec;
		delete[] DMshapeNorm;

		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract Yearly DM//////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


	if(((MNStruct *)globalcontext)->yearlyDM == 1){
		double yearlyamp=pow(10.0,Cube[pcount]);
		pcount++;
		double yearlyphase=Cube[pcount];
		pcount++;
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]-= yearlyamp*sin((2*M_PI/365.25)*(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat + yearlyphase)*DMVec[o];
		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract Solar Wind//////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////// 

	if(((MNStruct *)globalcontext)->FitSolarWind == 1){

		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){

			Resvec[o]-= (SolarWind-((MNStruct *)globalcontext)->pulse->ne_sw)*((MNStruct *)globalcontext)->pulse->obsn[o].tdis2;
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Band DM/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


        if(((MNStruct *)globalcontext)->incDMScatter == 1 || ((MNStruct *)globalcontext)->incDMScatter == 2  || ((MNStruct *)globalcontext)->incDMScatter == 3 ){

                if(((MNStruct *)globalcontext)->incDM == 0){
                        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                                DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                        }
                }



		double startfreq=0;
		double stopfreq=0;


		if(((MNStruct *)globalcontext)->incDMScatter == 1){		
			startfreq = 0;
			stopfreq=1000;
		}
		else  if(((MNStruct *)globalcontext)->incDMScatter == 2){
	                startfreq = 1000;
	                stopfreq=1800;
		}
	        else if(((MNStruct *)globalcontext)->incDMScatter == 3){
	                startfreq = 1800;
	                stopfreq=10000;
	        }



		double DMamp=Cube[pcount];
		pcount++;
		double DMindex=Cube[pcount];
		pcount++;

		double Tspan = maxtspan;
		double f1yr = 1.0/3.16e7;

		DMamp=pow(10.0, DMamp);
		if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(DMamp); }
		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (DMamp*DMamp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-DMindex))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}

		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;	
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos += FitDMScatterCoeff;

    	}


        if(((MNStruct *)globalcontext)->incDMScatter == 4 ){


                if(((MNStruct *)globalcontext)->incDM == 0){
                        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                                DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                        }
                }



		double Amp1=Cube[pcount];
		pcount++;
		double index1=Cube[pcount];
		pcount++;

		double Amp2=Cube[pcount];
		pcount++;
		double index2=Cube[pcount];
		pcount++;

		double Amp3=Cube[pcount];
		pcount++;
		double index3=Cube[pcount];
		pcount++;
		
		double Tspan = maxtspan;
		double f1yr = 1.0/3.16e7;
		

		Amp1=pow(10.0, Amp1);
		Amp2=pow(10.0, Amp2);
		Amp3=pow(10.0, Amp3);
		//Amp2=pow(10.0, -20.0);
		//Amp3=pow(10.0, -20.0);
		
//		 uniformpriorterm += log(Amp3) + log(Amp2);
		if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(Amp1) + log(Amp2) + log(Amp3); }


		double startfreq=0;
		double stopfreq=0;

		/////////////////////////Amp1/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 0;
		stopfreq=1000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
//		        double rho = (Amp1*Amp1/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			double rho = (Amp1*Amp1/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}

		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);///(pow(sqrt(DMKappa)*((double)((MNStruct *)globalcontext)->pulse->obsn[k].freqSSB),2));

					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);///(pow(sqrt(DMKappa)*((double)((MNStruct *)globalcontext)->pulse->obsn[k].freqSSB),2));
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////Amp2/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 1000;
		stopfreq=2000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			//double rho = (Amp2*Amp2/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);
			double rho = (Amp2*Amp2/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index2))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);///(pow(sqrt(DMKappa)*((double)((MNStruct *)globalcontext)->pulse->obsn[k].freqSSB),2));
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);///(pow(sqrt(DMKappa)*((double)((MNStruct *)globalcontext)->pulse->obsn[k].freqSSB),2));
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////Amp3/////////////////////////////////////////////////////////////////////////////////////////////


		startfreq = 2000;
		stopfreq= 10000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
//			double rho = (Amp3*Amp3/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);
			double rho = (Amp3*Amp3/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index3))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);///(pow(sqrt(DMKappa)*((double)((MNStruct *)globalcontext)->pulse->obsn[k].freqSSB),2));
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);///(pow(sqrt(DMKappa)*((double)((MNStruct *)globalcontext)->pulse->obsn[k].freqSSB),2));
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}

		startpos=startpos+FitDMScatterCoeff;
	}	


        if(((MNStruct *)globalcontext)->incDMScatter == 5 ){

	        
		if(((MNStruct *)globalcontext)->incDM == 0){
			for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		       		DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
	      		}
		}

		double Amp1=Cube[pcount];
		pcount++;
		double index1=Cube[pcount];
		pcount++;


		double Tspan = maxtspan;
		double f1yr = 1.0/3.16e7;
		

		Amp1=pow(10.0, Amp1);

		if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(Amp1); }


		double startfreq=0;
		double stopfreq=0;

		/////////////////////////50cm/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 0;
		stopfreq=1000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp1*Amp1)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}

		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////20cm/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 1000;
		stopfreq=1800;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp1*Amp1)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////10cm/////////////////////////////////////////////////////////////////////////////////////////////


		startfreq = 1800;
		stopfreq=10000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp1*Amp1)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}

		startpos=startpos+FitDMScatterCoeff;
	}	





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Add Group Noise/////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

        if(((MNStruct *)globalcontext)->incGroupNoise > 0){

		for(int g = 0; g < ((MNStruct *)globalcontext)->incGroupNoise; g++){

			int GrouptoFit=0;
			double GroupStartTime = 0;
			double GroupStopTime = 0;
			if(((MNStruct *)globalcontext)->FitForGroup[g][0] == -1){
				GrouptoFit = floor(Cube[pcount]);
				pcount++;
			}
			else{
				GrouptoFit = ((MNStruct *)globalcontext)->FitForGroup[g][0];
				
			}

                        if(((MNStruct *)globalcontext)->FitForGroup[g][1] == 1){
                                GroupStartTime = Cube[pcount];
                                pcount++;
				GroupStopTime = Cube[pcount];
                                pcount++;

                        }
                        else{
                                GroupStartTime = 0;
				GroupStopTime = 10000000.0;

                        }

		

			double GroupAmp=Cube[pcount];
			pcount++;
			double GroupIndex=Cube[pcount];
			pcount++;


		
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
		

			GroupAmp=pow(10.0, GroupAmp);
			if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(GroupAmp); }

			for (int i=0; i<FitGroupNoiseCoeff/2; i++){

				freqs[startpos+i]=((double)(i+1.0))/maxtspan;
				freqs[startpos+i+FitGroupNoiseCoeff/2]=freqs[startpos+i];
			
				double rho = (GroupAmp*GroupAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-GroupIndex))/(maxtspan*24*60*60);	
				powercoeff[startpos+i]+=rho;
				powercoeff[startpos+i+FitGroupNoiseCoeff/2]+=rho;
			}
		
		
			
			for (int i=0; i<FitGroupNoiseCoeff/2; i++){
				freqdet=freqdet+2*log(powercoeff[startpos+i]);
			}


			for(int i=0;i<FitGroupNoiseCoeff/2;i++){
				for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
					//printf("Groups %i %i %i %i %g %g %g\n", i,k,GrouptoFit,((MNStruct *)globalcontext)->sysFlags[k],(double) ((MNStruct *)globalcontext)->pulse->obsn[k].bat, GroupStartTime, GroupStopTime);
				if(((MNStruct *)globalcontext)->GroupNoiseFlags[k] == GrouptoFit && (double)((MNStruct *)globalcontext)->pulse->obsn[k].bat > GroupStartTime && (double)((MNStruct *)globalcontext)->pulse->obsn[k].bat < GroupStopTime){
//					if(((MNStruct *)globalcontext)->GroupNoiseFlags[k] == GrouptoFit){
				       		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				        	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);
						FMatrix[k][startpos+i+FitGroupNoiseCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);
					}
					else{	
						FMatrix[k][startpos+i]=0;
						FMatrix[k][startpos+i+FitGroupNoiseCoeff/2]=0;
					}
				}
			}



			startpos=startpos+FitGroupNoiseCoeff;
		}

    }






/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Add ECORR Coeffs////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	if(((MNStruct *)globalcontext)->incNGJitter >0){
		for(int i =0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
			powercoeff[startpos+i] = ECORRPrior[i];
			freqdet = freqdet + log(ECORRPrior[i]);
		}

		for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
			for(int i=0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
				FMatrix[k][startpos+i] = ((MNStruct *)globalcontext)->NGJitterMatrix[k][i];
			}
		}

	}






/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Time domain likelihood//////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

/*
	static unsigned int oldcw;
	if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
		fpu_fix_start(&oldcw);
	//	printf("oldcw %i \n", oldcw);
	}
*/	


	double tdet=0;
	double timelike=0;

	for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		//printf("Res: %i  %g %g \n", o, Resvec[o], 1.0/sqrt(Noise[o]));
		timelike+=Resvec[o]*Resvec[o]*Noise[o];
		tdet -= log(Noise[o]);
	}

/*
	dd_real ddtimelike=0.0;
	if(((MNStruct *)globalcontext)->uselongdouble ==1){
		for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			dd_real res = (dd_real)Resvec[o];
			dd_real Nval = (dd_real)Noise[o];
			dd_real chicomp = res*res*Nval;
		//	printf("Res %i %g %g \n", o, cast2double(res), cast2double(res*res*Nval));
			ddtimelike += chicomp;
		}
	}
	//printf("timelike %g %g \n", timelike, cast2double(ddtimelike));
	
        qd_real qdtimelike=0.0;
        if(((MNStruct *)globalcontext)->uselongdouble ==2){
                for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){

                        qd_real res = (qd_real)Resvec[o];
                        qd_real Nval = (qd_real)Noise[o];
                        qd_real chicomp = res*res*Nval;

                        qdtimelike+=chicomp;
                }
		//printf("qdtimelike %15.10e\n", qdtimelike.x[0]);
        }

*/

//////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////Form Total Matrices////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

	int totalsize=TimetoMargin+totCoeff+totalredshapecoeff;

/*
	double **TotalMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i =0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		TotalMatrix[i]=new double[totalsize];
		for(int j =0;j<totalsize; j++){
			TotalMatrix[i][j]=0;
		}
	}
	
	
	for(int i =0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		for(int j =0;j<TimetoMargin; j++){
			TotalMatrix[i][j]= TNDM[i + j*((MNStruct *)globalcontext)->pulse->nobs];
		}
		
		for(int j =0;j<totCoeff; j++){
			TotalMatrix[i][j+TimetoMargin]=FMatrix[i][j];
		}

		for(int j =0;j<totalredshapecoeff; j++){
			TotalMatrix[i][j+TimetoMargin+totCoeff]=RedShapeletMatrix[i][j];
			//printf("Red shape: %i %i %g \n", i, j, RedShapeletMatrix[i][j]);
		}
	}

        for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
                delete[] FMatrix[j];
        }
        delete[] FMatrix;
//	if(TimetoMargin != ((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps){
//		for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
  //              	delete[] TNDM[j];
    //    	}
        	delete[] TNDM;
//	}


*/

	double *TotalMatrix=new double[((MNStruct *)globalcontext)->pulse->nobs*totalsize];
	for(int i =0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		for(int j =0;j<totalsize; j++){
			TotalMatrix[i + j*((MNStruct *)globalcontext)->pulse->nobs]=0;
		}
	}
	
	
	for(int i =0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		for(int j =0;j<TimetoMargin; j++){
			TotalMatrix[i + j*((MNStruct *)globalcontext)->pulse->nobs]= TNDM[i + j*((MNStruct *)globalcontext)->pulse->nobs];
		}
		
		for(int j =0;j<totCoeff; j++){
			TotalMatrix[i + (j+TimetoMargin)*((MNStruct *)globalcontext)->pulse->nobs]=FMatrix[i][j];
		}

		for(int j =0;j<totalredshapecoeff; j++){
			TotalMatrix[i + (j+TimetoMargin+totCoeff)*((MNStruct *)globalcontext)->pulse->nobs]=RedShapeletMatrix[i][j];
			//printf("Red shape: %i %i %g \n", i, j, RedShapeletMatrix[i][j]);
		}
	}

        for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
                delete[] FMatrix[j];
        }
        delete[] FMatrix;
//	if(TimetoMargin != ((MNStruct *)globalcontext)->numFitTiming+((MNStruct *)globalcontext)->numFitJumps){
//		for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
  //              	delete[] TNDM[j];
    //    	}
        	delete[] TNDM;
//	}


//////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////Do Algebra/////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

	double *NTd = new double[totalsize];

/*	double **NT=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		NT[i]=new double[totalsize];
	}

	double **TNT=new double*[totalsize];
	for(int i=0;i<totalsize;i++){
		TNT[i]=new double[totalsize];
	}
	
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		for(int j=0;j<totalsize;j++){
			NT[i][j]=TotalMatrix[i][j]*Noise[i];
		}
	}
*/
	

	double *NT=new double[((MNStruct *)globalcontext)->pulse->nobs*totalsize];
	double *TNT=new double[totalsize*totalsize];
	
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		for(int j=0;j<totalsize;j++){
			NT[i + j*((MNStruct *)globalcontext)->pulse->nobs]=TotalMatrix[i + j*((MNStruct *)globalcontext)->pulse->nobs]*Noise[i];
		}
	}

	vector_dgemm(TotalMatrix, NT , TNT, ((MNStruct *)globalcontext)->pulse->nobs, totalsize, ((MNStruct *)globalcontext)->pulse->nobs, totalsize, 'T', 'N');

	vector_dgemv(NT,Resvec,NTd,((MNStruct *)globalcontext)->pulse->nobs,totalsize,'T');

/*
        dd_real ddfreqlike = 0.0;
        dd_real ddsigmadet = 0.0;

        dd_real ddfreqlikeChol = 0.0;
        dd_real ddsigmadetChol = 0.0;

	int ddinfo = 0;
	if(((MNStruct *)globalcontext)->uselongdouble ==1){
		mpackint n = totalsize;
		mpackint m = ((MNStruct *)globalcontext)->pulse->nobs;


		dd_real *A = new dd_real[m * n];
		dd_real *B = new dd_real[m * n];
		dd_real *C = new dd_real[n * n];
		dd_real *CholC = new dd_real[n * n];

		dd_real *ddRes = new dd_real[m];
		dd_real *ddNTd = new dd_real[n];
		dd_real *ddNTdCopy = new dd_real[n];
		dd_real *ddPC = new dd_real[totCoeff];
		dd_real alpha, beta;


		for (int i=0; i<m; i++) for (int j=0; j<n; j++) A[i+j*m] = (dd_real) TotalMatrix[i+j*m];
		for (int i=0; i<m; i++) for (int j=0; j<n; j++) B[i+j*m] = (dd_real) NT[i+j*m];
		for(int i =0; i < m; i++) ddRes[i] = (dd_real)Resvec[i];
		for(int i =0; i < totCoeff; i++) ddPC[i] = (dd_real) (1.0/powercoeff[i]);

		alpha = 1.0;
		beta =  0.0;
		Rgemv("t", m,n,alpha, B, m, ddRes, 1, beta, ddNTd, 1);
		for(int i =0; i < n; i++) {
			ddNTdCopy[i] = ddNTd[i];
			//printf("ddNT %i %g \n", i, cast2double(ddNTd[i]));
		}


		Rgemm("t", "n", n, n, m, alpha, A, m, B, m, beta, C, n);



		for(int j=0;j<totCoeff;j++){
			int l = TimetoMargin+j;
			C[l+l*n] += ddPC[j];
		}

		for(int j=0;j<totalredshapecoeff;j++){
			int l = TimetoMargin+totCoeff+j;
			dd_real detfac = (dd_real)pow(10.0, -12);
			C[l+l*n] += detfac*C[l+l*n];
			
		}

		for(int i =0; i < n*n; i++)CholC[i] = C[i];

		mpackint lwork, info;

    		mpackint *ipiv = new mpackint[n];



		Rgetrf(n, n, C, n, ipiv, &info);
		if(info != 0)ddinfo = (int)info;
		for(int i =0; i < n; i++) ddsigmadet += log(abs(C[i+i*n]));

		Rgetrs("n", n, 1, C, n, ipiv, ddNTdCopy, n, &info);

		if(info != 0)ddinfo = (int)info;

		for(int i =0; i < totalsize; i++){
			ddfreqlike += ddNTd[i]*ddNTdCopy[i];
		}


		int printResiduals=0;

		if(printResiduals==1){
			dd_real *ddSignal = new dd_real[m];
			Rgemv("n", m,n,alpha, A, m, ddNTdCopy, 1, beta, ddSignal, 1);

			for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
				printf("%.10Lg %g %g \n", ((MNStruct *)globalcontext)->pulse->obsn[i].bat, (double) ((MNStruct *)globalcontext)->pulse->obsn[i].residual, cast2double(ddSignal[i]));
			}
			
			delete[] ddSignal;
		}

			
		delete[] ddPC;
		delete[] ddNTd;
		delete[] ddNTdCopy;
		delete[]C;
		delete[]A;
		delete[] ipiv;



                dd_real *ddNTdChol = new dd_real[n];
                dd_real *ddNTdCholCopy = new dd_real[n];

		Rgemv("t", m,n,alpha, B, m, ddRes, 1, beta, ddNTdChol, 1);
                for(int i =0; i < n; i++) ddNTdCholCopy[i] = ddNTdChol[i];


                mpackint Cholinfo;


                Rpotrf("u", n, CholC, n, &Cholinfo);
                if(Cholinfo != 0)ddinfo = (int)Cholinfo;
                for(int i =0; i < n; i++) ddsigmadetChol += log(abs(CholC[i+i*n]));

                Rpotrs("u", n, 1, CholC, n,  ddNTdCholCopy, n, &Cholinfo);

                if(Cholinfo != 0)ddinfo = (int)Cholinfo;

                for(int i =0; i < totalsize; i++){
                        ddfreqlikeChol += ddNTdChol[i]*ddNTdCholCopy[i];
                }


                delete[] ddNTdChol;
                delete[] ddNTdCholCopy;
                delete[]CholC;
		delete[] ddRes;
		delete[] B;
	}


        qd_real qdfreqlike = 0.0;
        qd_real qdsigmadet = 0.0;

        qd_real qdfreqlikeChol = 0.0;
        qd_real qdsigmadetChol = 0.0;

	int qdinfo = 0;
	if(((MNStruct *)globalcontext)->uselongdouble ==2){
		mpackint n = totalsize;
		mpackint m = ((MNStruct *)globalcontext)->pulse->nobs;


		qd_real *A = new qd_real[m * n];
		qd_real *B = new qd_real[m * n];
		qd_real *C = new qd_real[n * n];
		qd_real *CholC = new qd_real[n * n];


		qd_real *qdRes = new qd_real[m];
		qd_real *qdNTd = new qd_real[n];
		qd_real *qdNTdCopy = new qd_real[n];
		qd_real *qdPC = new qd_real[totCoeff];
		qd_real alpha, beta;


		for (int i=0; i<m; i++) for (int j=0; j<n; j++) A[i+j*m] = (qd_real) TotalMatrix[i+j*m];
		for (int i=0; i<m; i++) for (int j=0; j<n; j++) B[i+j*m] = (qd_real) NT[i+j*m];
		for(int i =0; i < m; i++) qdRes[i] = (qd_real) Resvec[i];
		for(int i =0; i < totCoeff; i++) qdPC[i] = (qd_real) (1.0/powercoeff[i]);
		alpha = 1.0;
		beta =  0.0;

		Rgemv("t", m,n,alpha, B, m, qdRes, 1, beta, qdNTd, 1);
                for(int i =0; i < n; i++) qdNTdCopy[i] = qdNTd[i];

		Rgemm("t", "n", n, n, m, alpha, A, m, B, m, beta, C, n);



		for(int j=0;j<totCoeff;j++){
				int l = TimetoMargin+j;
				//printf("FNF %i %25.15e %g \n", j, C[l+l*n].x[0], cast2double(qdPC[j]));
				C[l+l*n] += qdPC[j];
		}

		for(int j=0;j<totalredshapecoeff;j++){
			int l = TimetoMargin+totCoeff+j;
			qd_real detfac = (qd_real)pow(10.0, -12);
			C[l+l*n] += detfac*C[l+l*n];
			
		}

		for(int i =0; i < n*n; i++)CholC[i] = C[i];

		mpackint lwork, info;

    		mpackint *ipiv = new mpackint[n];



		Rgetrf(n, n, C, n, ipiv, &info);
		if(info != 0)qdinfo = (int)info;
		for(int i =0; i < n; i++) qdsigmadet += log(abs(C[i+i*n]));

		Rgetrs("n", n, 1, C, n, ipiv, qdNTdCopy, n, &info);

		if(info != 0)qdinfo = (int)info;

		for(int i =0; i < totalsize; i++){
			qdfreqlike += qdNTd[i]*qdNTdCopy[i];
		}

			
		delete[] qdPC;
		delete[] qdNTd;
		delete[]C;
		delete[]A;
		delete[] ipiv;


                qd_real *qdNTdChol = new qd_real[n];
                qd_real *qdNTdCholCopy = new qd_real[n];



                mpackint Cholinfo;

                Rgemv("t", m,n,alpha, B, m, qdRes, 1, beta, qdNTdChol, 1);
                for(int i =0; i < n; i++) qdNTdCholCopy[i] = qdNTdChol[i];


                Rpotrf("u", n, CholC, n, &Cholinfo);
                if(Cholinfo != 0)qdinfo = (int)Cholinfo;
                for(int i =0; i < n; i++) qdsigmadetChol += log(abs(CholC[i+i*n]));

                Rpotrs("u", n, 1, CholC, n,  qdNTdCholCopy, n, &Cholinfo);

                if(Cholinfo != 0)qdinfo = (int)Cholinfo;

                for(int i =0; i < totalsize; i++){
                        qdfreqlikeChol += qdNTdChol[i]*qdNTdCholCopy[i];
		//	printf("diff %i %g %g %g \n", i, cast2double(qdNTdCholCopy[i] - qdNTdCopy[i]), cast2double(qdNTdCholCopy[i]), cast2double(qdNTdCopy[i]));
                }

		 delete[] qdNTdCopy;
		delete[] B;
		delete[] qdRes;
                delete[] qdNTdChol;
                delete[] qdNTdCholCopy;
                delete[]CholC;

	}


*/
	for(int j=0;j<totCoeff;j++){
			TNT[TimetoMargin+j + (TimetoMargin+j)*totalsize] += 1.0/powercoeff[j];
	}
	for(int j=0;j<totalredshapecoeff;j++){
			freqdet=freqdet-log(pow(10.0, -12)*TNT[TimetoMargin+totCoeff+j + (TimetoMargin+totCoeff+j)*totalsize]);
			TNT[TimetoMargin+totCoeff+j + (TimetoMargin+totCoeff+j)*totalsize] += pow(10.0, -12)*TNT[TimetoMargin+totCoeff+j + (TimetoMargin+totCoeff+j)*totalsize];
			
	}

	double freqlike=0;
	double *WorkCoeff = new double[totalsize];
	double *WorkCoeff2 = new double[totalsize];
	double *TNT2=new double[totalsize*totalsize];
	
	for(int i =0; i < totalsize; i++){
//		TNT2[i] = new double[totalsize];
		for(int j=0 ; j < totalsize; j++){
//			TNT2[i][j] = TNT[i][j];
			TNT2[i + j*totalsize] = TNT[i + j*totalsize];			
		}
	}
	for(int o1=0;o1<totalsize; o1++){
		
		WorkCoeff[o1]=NTd[o1];
		WorkCoeff2[o1]=NTd[o1];
	}

	int globalinfo=0;
	int info=0;
	double jointdet = 0;	
	vector_dpotrfInfo(TNT, totalsize, jointdet, info);
	if(info != 0)globalinfo=1;

	info=0;
	vector_dpotrsInfo(TNT, WorkCoeff, totalsize, info);

	/*
	if(((MNStruct *)globalcontext)->doGrades == 1){

		for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
			((MNStruct *)globalcontext)->PreviousNoise[i] = Noise[i];
		}

		for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
			for(int j=0;j<totalsize;j++){
				((MNStruct *)globalcontext)->PreviousNT[i][j] = NT[i][j];
			}
		}

		for(int i =0; i < totalsize; i++){
			for(int j=0 ; j < totalsize; j++){
				((MNStruct *)globalcontext)->PreviousTNT[i][j] = TNT[i][j];
			}
		}
	}
*/
        if(info != 0)globalinfo=1;
	info=0;
	double det2=0;
	vector_TNqrsolve(TNT2, NTd, WorkCoeff2, totalsize, det2, info);
 
	double freqlike2 = 0;    
	for(int j=0;j<totalsize;j++){
		freqlike += NTd[j]*WorkCoeff[j];
		freqlike2+=NTd[j]*WorkCoeff2[j];
	}


	double lnewChol =-0.5*(tdet+jointdet+freqdet+timelike-freqlike) + uniformpriorterm;
	double lnew=-0.5*(tdet+det2+freqdet+timelike-freqlike2) + uniformpriorterm;
	//printf("Double %g %i\n", lnew, globalinfo);
	if(fabs(lnew-lnewChol)>0.05){
		globalinfo = 1;
	//	lnew=-pow(10.0,20);
	}
	if(isnan(lnew) || isinf(lnew) || globalinfo != 0){
		globalinfo = 1;
		lnew=-pow(10.0,20);
		
	}
	//printf("lnew double : %g", lnew);
/*	if(((MNStruct *)globalcontext)->uselongdouble ==1){

		dd_real ddAllLike = -0.5*(tdet+ddsigmadet+freqdet+ddtimelike-ddfreqlike) + uniformpriorterm;
		double ddlike = cast2double(ddAllLike);
		dd_real ddAllLikeChol = -0.5*(tdet+2*ddsigmadetChol+freqdet+ddtimelike-ddfreqlikeChol) + uniformpriorterm;
		double ddlikeChol = cast2double(ddAllLikeChol);
		lnew = ddlike;
		
		if(fabs(ddlikeChol-ddlike)>0.5){
			lnew=-pow(10.0,20);
		}


		if(isnan(lnew) || isinf(lnew) || ddinfo != 0){
			lnew=-pow(10.0,20);
		}

	//	printf("double double %g %g %g\n", lnew,ddlike,  fabs(ddlikeChol-ddlike));
	}

	

        if(((MNStruct *)globalcontext)->uselongdouble ==2){

		qd_real qdAllLike = -0.5*(tdet+qdsigmadet+freqdet+qdtimelike-qdfreqlike) + uniformpriorterm;
		double qdlike = cast2double(qdAllLike);
		qd_real qdAllLikeChol = -0.5*(tdet+2*qdsigmadetChol+freqdet+qdtimelike-qdfreqlikeChol) + uniformpriorterm;
		double qdlikeChol = cast2double(qdAllLikeChol);

                lnew = qdlike;

                if(fabs(qdlikeChol-qdlike)>0.05){
                        lnew=-pow(10.0,20);
                }


                if(isnan(lnew) || isinf(lnew) || ddinfo != 0){
                        lnew=-pow(10.0,20);
                }
        }


        if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
                fpu_fix_end(&oldcw);
        }
*/
	if(badshape == 1){lnew=-pow(10.0,20);}


	delete[] DMVec;
	delete[] WorkCoeff;
	delete[] WorkCoeff2;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] powercoeff;
	delete[] NTd;
	delete[] freqs;
	delete[] Noise;
	delete[] Resvec;
	
//	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
//		delete[] TotalMatrix[j];
//		delete[] NT[j];
		

//	}
	delete[] TotalMatrix;
	delete[] NT;
	

//	for (int j = 0; j < totalsize; j++){
//		delete[]TNT[j];
//		delete[] TNT2[j];
//	}
	delete[]TNT;
	delete[] TNT2;

	if(((MNStruct *)globalcontext)->incNGJitter >0){
		delete[] ECORRPrior;
	}

	if(totalredshapecoeff > 0){
		for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
			delete[] RedShapeletMatrix[j];
		}
		delete[] RedShapeletMatrix;
	}


        ((MNStruct *)globalcontext)->PreviousInfo = globalinfo;
        ((MNStruct *)globalcontext)->PreviousJointDet = jointdet;
        ((MNStruct *)globalcontext)->PreviousFreqDet = freqdet;
        ((MNStruct *)globalcontext)->PreviousUniformPrior = uniformpriorterm;

        // printf("tdet %g, jointdet %g, freqdet %g, lnew %g, timelike %g, freqlike %g\n", tdet, jointdet, freqdet, lnew, timelike, freqlike);


	//printf("CPUChisq: %g %g %g %g %g %g \n",lnew,jointdet,tdet,freqdet,timelike,freqlike);


	return lnew;

	


}


double  LRedNumericalLogLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	double **EFAC;
	double *EQUAD;

	
	int numfit=((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps;
	long double LDparams[numfit];




	int fitcount=0;
	int pcount=0;
	
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){
		if(((MNStruct *)globalcontext)->Dpriors[p][1] != ((MNStruct *)globalcontext)->Dpriors[p][0]){

			double val = 0;
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 0){
				val = Cube[fitcount];
			}
			if((((MNStruct *)globalcontext)->LDpriors[p][3]) == 1){
				val = pow(10.0,Cube[fitcount]);
			}
			LDparams[p]=val*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
			fitcount++;
		}
		else if(((MNStruct *)globalcontext)->Dpriors[p][1] == ((MNStruct *)globalcontext)->Dpriors[p][0]){
			LDparams[p]=((MNStruct *)globalcontext)->Dpriors[p][0]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		}


	}

	pcount=0;
	double phase=(double)LDparams[0];
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	
	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,1);       /* Form residuals */
	

	double *Resvec=new double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		Resvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].residual+phase;

	}
	
	
	
	pcount=fitcount;


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract Steps////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	if(((MNStruct *)globalcontext)->incStep > 0){
		for(int i = 0; i < ((MNStruct *)globalcontext)->incStep; i++){
			double StepAmp = Cube[pcount];
			pcount++;
			double StepTime = Cube[pcount];
			pcount++;
			for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
				if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > StepTime){
					Resvec[o1] += StepAmp;
				}
			}
		}
	}	


/////////////////////////////////////////////////////////////////////////////////////////////  
///////////////////////////Subtract GLitches/////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


 
	for(int i = 0 ; i < ((MNStruct *)globalcontext)->incGlitch; i++){
		double GlitchMJD = Cube[pcount];
		pcount++;

		double *GlitchAmps = new double[2];
		if(((MNStruct *)globalcontext)->incGlitchTerms==1 || ((MNStruct *)globalcontext)->incGlitchTerms==3){
			GlitchAmps[0]  = Cube[pcount];
			pcount++;
		}
		else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
			GlitchAmps[0]  = Cube[pcount];
                        pcount++;
			GlitchAmps[1]  = Cube[pcount];
                        pcount++;
		}


		for(int o1=0;o1<((MNStruct *)globalcontext)->pulse->nobs; o1++){
                        if(((MNStruct *)globalcontext)->pulse->obsn[o1].bat > GlitchMJD){

				if(((MNStruct *)globalcontext)->incGlitchTerms==1){

					long double arg=0;
					arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;

				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==2){
					for(int j = 0; j < ((MNStruct *)globalcontext)->incGlitchTerms; j++){

						long double arg=0;
						if(j==0){
							arg=((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0])*86400.0;
						}
						if(j==1){
							arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
						}
						double darg = (double)arg;
						Resvec[o1] += GlitchAmps[j]*darg;
					}
				}
				else if(((MNStruct *)globalcontext)->incGlitchTerms==3){
					long double arg=0;
					arg=0.5*pow((((MNStruct *)globalcontext)->pulse->obsn[o1].bat - GlitchMJD)*86400.0,2)/((MNStruct *)globalcontext)->pulse->param[param_f].val[0];
					double darg = (double)arg;
                                        Resvec[o1] += GlitchAmps[0]*darg;
				}


			}
		}
	}
	
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get White Noise vector///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	double uniformpriorterm=0;

	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
					EFAC[n-1][o]=1;
				}
			}
			else{
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                   EFAC[n-1][o]=0;
                }
			}
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
					EFAC[n-1][o]=pow(10.0,Cube[pcount]);
					if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[n-1][o]);}
				}
				pcount++;
			}
			else{
                                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){

                                        EFAC[n-1][o]=pow(10.0,Cube[pcount]);
                                }
                                pcount++;
                        }
		}
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double*[((MNStruct *)globalcontext)->EPolTerms];
		for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
			EFAC[n-1]=new double[((MNStruct *)globalcontext)->systemcount];
			if(n==1){
				for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
					EFAC[n-1][p]=pow(10.0,Cube[pcount]);
					if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[n-1][p]);}
					pcount++;
				}
			}
			else{
                                for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
                                        EFAC[n-1][p]=pow(10.0,Cube[pcount]);
                                        pcount++;
                                }
                        }
		}
	}	

		
	//printf("Equad %i \n", ((MNStruct *)globalcontext)->numFitEQUAD);
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,2*Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount]));}
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){

			if(((MNStruct *)globalcontext)->includeEQsys[o] == 1){
				//printf("Cube: %i %i %g \n", o, pcount, Cube[pcount]);
				EQUAD[o]=pow(10.0,2*Cube[pcount]);
				if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount])); }
				pcount++;
			}
			else{
				EQUAD[o]=0;
			}
			//printf("Equad? %i %g \n", o, EQUAD[o]);
		}
    	}
    

    double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,2*Cube[pcount]);
			if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount]));}
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
            SQUAD[o]=pow(10.0,2*Cube[pcount]);
	    if(((MNStruct *)globalcontext)->EQUADPriorType ==1) { uniformpriorterm +=log(pow(10.0,Cube[pcount])); }

			pcount++;
        }
    }
 
	double *ECORRPrior;
	if(((MNStruct *)globalcontext)->incNGJitter >0){
		double *ECorrCoeffs=new double[((MNStruct *)globalcontext)->incNGJitter];	
		for(int i =0; i < ((MNStruct *)globalcontext)->incNGJitter; i++){
			ECorrCoeffs[i] = pow(10.0, 2*Cube[pcount]);
			pcount++;
		}
    		ECORRPrior = new double[((MNStruct *)globalcontext)->numNGJitterEpochs];
		for(int i =0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
			ECORRPrior[i] = ECorrCoeffs[((MNStruct *)globalcontext)->NGJitterSysFlags[i]];
		}

		delete[] ECorrCoeffs;
	} 



	double *Noise;	
	double *BATvec;
	Noise=new double[((MNStruct *)globalcontext)->pulse->nobs];
	BATvec=new double[((MNStruct *)globalcontext)->pulse->nobs];
	
	
	for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		BATvec[o]=(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat;
	}
		
		
	if(((MNStruct *)globalcontext)->whitemodel == 0){
	
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			double EFACterm=0;
			double noiseval=0;
			double ShannonJitterTerm=0;
			
			
			if(((MNStruct *)globalcontext)->useOriginalErrors==0){
				noiseval=((MNStruct *)globalcontext)->pulse->obsn[o].toaErr;
			}
			else if(((MNStruct *)globalcontext)->useOriginalErrors==1){
				noiseval=((MNStruct *)globalcontext)->pulse->obsn[o].origErr;
			}


			for(int n=1; n <=((MNStruct *)globalcontext)->EPolTerms; n++){
				EFACterm=EFACterm + pow((noiseval*pow(10.0,-6))/pow(pow(10.0,-7),n-1),n)*EFAC[n-1][((MNStruct *)globalcontext)->sysFlags[o]];
			}	
			
			if(((MNStruct *)globalcontext)->incShannonJitter > 0){	
			 	ShannonJitterTerm=SQUAD[((MNStruct *)globalcontext)->sysFlags[o]]*((MNStruct *)globalcontext)->TobsInfo[o]/1000.0;
			}

			Noise[o]= 1.0/(pow(EFACterm,2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]+ShannonJitterTerm);

		}
		
	}
	else if(((MNStruct *)globalcontext)->whitemodel == 1){
	
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){

			Noise[o]=1.0/(EFAC[0][((MNStruct *)globalcontext)->sysFlags[o]]*EFAC[0][((MNStruct *)globalcontext)->sysFlags[o]]*(pow(((((MNStruct *)globalcontext)->pulse->obsn[o].toaErr)*pow(10.0,-6)),2) + EQUAD[((MNStruct *)globalcontext)->sysFlags[o]]));
		}
		
	}

	
	

//////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////////////Set up Coefficients///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////  



	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }

	double maxtspan=1*(end-start);
	double averageTSamp=2*maxtspan/((MNStruct *)globalcontext)->pulse->nobs;

	double **DMEventInfo;

	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int FitDMCoeff=2*(((MNStruct *)globalcontext)->numFitDMCoeff);
	int FitDMScatterCoeff=2*(((MNStruct *)globalcontext)->numFitDMScatterCoeff);
	int FitGroupNoiseCoeff = 2*((MNStruct *)globalcontext)->numFitGroupNoiseCoeff;

	if(((MNStruct *)globalcontext)->incFloatDM != 0)FitDMCoeff+=2*((MNStruct *)globalcontext)->incFloatDM;
	if(((MNStruct *)globalcontext)->incFloatRed != 0)FitRedCoeff+=2*((MNStruct *)globalcontext)->incFloatRed;
	int DMEventCoeff=0;
	if(((MNStruct *)globalcontext)->incDMEvent != 0){
		DMEventInfo=new double*[((MNStruct *)globalcontext)->incDMEvent];
		for(int i =0; i < ((MNStruct *)globalcontext)->incDMEvent; i++){
			DMEventInfo[i]=new double[7];
			DMEventInfo[i][0]=Cube[pcount]; //Start time
			pcount++;
			DMEventInfo[i][1]=Cube[pcount]; //Length
			pcount++;
			DMEventInfo[i][2]=pow(10.0, Cube[pcount]); //Amplitude
			pcount++;
			DMEventInfo[i][3]=Cube[pcount]; //SpectralIndex
			pcount++;
			DMEventInfo[i][4]=Cube[pcount]; //Offset
			pcount++;
			DMEventInfo[i][5]=Cube[pcount]; //Linear
			pcount++;
			DMEventInfo[i][6]=Cube[pcount]; //Quadratic
			pcount++;
			DMEventCoeff+=2*int(DMEventInfo[i][1]/averageTSamp);

			}
	}


	int totCoeff=0;
	if(((MNStruct *)globalcontext)->incRED != 0)totCoeff+=FitRedCoeff;
	if(((MNStruct *)globalcontext)->incDM != 0)totCoeff+=FitDMCoeff;

	if(((MNStruct *)globalcontext)->incDMScatter == 1 || ((MNStruct *)globalcontext)->incDMScatter == 2  || ((MNStruct *)globalcontext)->incDMScatter == 3)totCoeff+=FitDMScatterCoeff;
	if(((MNStruct *)globalcontext)->incDMScatter == 4 ||((MNStruct *)globalcontext)->incDMScatter == 5 )totCoeff+=3*FitDMScatterCoeff;

	if(((MNStruct *)globalcontext)->incDMEvent != 0)totCoeff+=DMEventCoeff; 
	if(((MNStruct *)globalcontext)->incNGJitter >0)totCoeff+=((MNStruct *)globalcontext)->numNGJitterEpochs;

	if(((MNStruct *)globalcontext)->incGroupNoise > 0)totCoeff += ((MNStruct *)globalcontext)->incGroupNoise*FitGroupNoiseCoeff;





	double *powercoeff=new double[totCoeff];
	for(int o=0;o<totCoeff; o++){
		powercoeff[o]=0;
	}
	
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			FMatrix[i][j]=0;
		}
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Red Noise///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


    double *freqs = new double[totCoeff];

    double *DMVec=new double[((MNStruct *)globalcontext)->pulse->nobs];
    double *SignalCoeff = new double[totCoeff];

	for(int i = 0; i < totCoeff; i++){
		SignalCoeff[i] = 0;
	}


    double DMKappa = 2.410*pow(10.0,-16);
    int startpos=0;
    double freqdet=0;
    double GWBAmpPrior=0;
    int badAmp=0;
    if(((MNStruct *)globalcontext)->incRED==2){

    
		for (int i=0; i<FitRedCoeff/2; i++){
			int pnum=pcount;
			double pc=Cube[pcount];

			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm += log(pow(10.0,pc)); }

			freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
			freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];
			powercoeff[i]=pow(10.0,2*pc);
			powercoeff[i+FitRedCoeff/2]=powercoeff[i];
			freqdet=freqdet+2*log(powercoeff[i]);
			pcount++;
		}
		
		
        for(int i=0;i<FitRedCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);

                }
        }

        for(int i=0;i<FitRedCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
                }
        }
	            
	    startpos=FitRedCoeff;

    }
   else if(((MNStruct *)globalcontext)->incRED==5 || ((MNStruct *)globalcontext)->incRED==6){

		freqdet=0;
		if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 1){
			double fLow = pow(10.0, Cube[pcount]);
			pcount++;

			double deltaLogF = 0.1;
			double RedMidFreq = 2.0;

			double RedLogDiff = log10(RedMidFreq) - log10(fLow);
			int LogLowFreqs = floor(RedLogDiff/deltaLogF);

			double RedLogSampledDiff = LogLowFreqs*deltaLogF;
			double sampledFLow = floor(log10(fLow)/deltaLogF)*deltaLogF;
			
			int freqStartpoint = 0;


			for(int i =0; i < LogLowFreqs; i++){
				((MNStruct *)globalcontext)->sampleFreq[freqStartpoint]=pow(10.0, sampledFLow + i*RedLogSampledDiff/LogLowFreqs);
				freqStartpoint++;

			}

			for(int i =0;i < FitRedCoeff/2-LogLowFreqs; i++){
				((MNStruct *)globalcontext)->sampleFreq[freqStartpoint]=i+RedMidFreq;
				freqStartpoint++;
			}

		}

                if(((MNStruct *)globalcontext)->FitLowFreqCutoff == 2){
                        double fLow = pow(10.0, Cube[pcount]);
                        pcount++;

                        for(int i =0;i < FitRedCoeff/2; i++){
                                ((MNStruct *)globalcontext)->sampleFreq[i]=((double)(i+1))*fLow;
                        }

                }



		for(int i = 0; i < FitRedCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			pcount++;
		}
			
		
		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitRedPL; pl ++){

			double Tspan = maxtspan;
                        double f1yr = 1.0/3.16e7;


			double redamp=Cube[pcount];
			pcount++;
			double redindex=Cube[pcount];
			pcount++;

			
			double cornerfreq=0;
			if(((MNStruct *)globalcontext)->incRED==4){
				cornerfreq=pow(10.0, Cube[pcount])/Tspan;
				pcount++;
			}

	
			
			redamp=pow(10.0, redamp);
			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm +=log(redamp); }



			double Agw=redamp;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
				freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];
				
				double rho=0;
				if(((MNStruct *)globalcontext)->incRED==5){	
 					rho = (Agw*Agw/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-redindex))/(maxtspan*24*60*60);
				}
				if(((MNStruct *)globalcontext)->incRED==6){
					
			rho = pow((1+(pow((1.0/365.25)/cornerfreq,redindex/2))),2)*(Agw*Agw/12.0/(M_PI*M_PI))/pow((1+(pow(freqs[i]/cornerfreq,redindex/2))),2)/(maxtspan*24*60*60)*pow(f1yr,-3.0);
				}
				//if(rho > pow(10.0,15))rho=pow(10.0,15);
 				powercoeff[i]+= rho;
 				powercoeff[i+FitRedCoeff/2]+= rho;

				

			}
		}
		


		int coefftovary=0;
		double amptovary=0.0;
		if(((MNStruct *)globalcontext)->varyRedCoeff==1){
			coefftovary=int(pow(10.0,Cube[pcount]))-1;
			pcount++;
			amptovary=pow(10.0,Cube[pcount]);
			pcount++;

			powercoeff[coefftovary]=amptovary;
			powercoeff[coefftovary+FitRedCoeff/2]=amptovary;	
		}		
		
		double GWBAmp=0;
		if(((MNStruct *)globalcontext)->incGWB==1){
			GWBAmp=pow(10.0,Cube[pcount]);
			pcount++;
			GWBAmpPrior=log(GWBAmp);
			//uniformpriorterm +=GWBAmpPrior;
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				double rho = (GWBAmp*GWBAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-4.333))/(maxtspan*24*60*60);
				powercoeff[i]+= rho;
				powercoeff[i+FitRedCoeff/2]+= rho;
			}
		}

		for (int i=0; i<FitRedCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[i]);
		}
		
		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);

			}
		}

		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
			}
		}

		startpos=FitRedCoeff;

    }


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  Red Shape Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	double globalwidth=0;
	if(((MNStruct *)globalcontext)->incRedShapeEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incRedShapeEvent; i++){

			int numRedShapeCoeff=((MNStruct *)globalcontext)->numRedShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=Cube[pcount];
			pcount++;

			globalwidth=EventWidth;


			double *Redshapecoeff=new double[numRedShapeCoeff];
			double *RedshapeVec=new double[numRedShapeCoeff];
			for(int c=0; c < numRedShapeCoeff; c++){
				Redshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *RedshapeNorm=new double[numRedShapeCoeff];
			for(int c=0; c < numRedShapeCoeff; c++){
				RedshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numRedShapeCoeff,HVal,RedshapeVec);
				double Redsignal=0;
				for(int c=0; c < numRedShapeCoeff; c++){
					Redsignal += RedshapeNorm[c]*RedshapeVec[c]*Redshapecoeff[c];
				}

				  Resvec[k] -= Redsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] Redshapecoeff;
		delete[] RedshapeVec;
		delete[] RedshapeNorm;

		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract Sine Wave///////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	if(((MNStruct *)globalcontext)->incsinusoid == 1){
		double sineamp=pow(10.0,Cube[pcount]);
		pcount++;
		double sinephase=Cube[pcount];
		pcount++;
		double sinefreq=pow(10.0,Cube[pcount])/maxtspan;
		pcount++;		
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]-= sineamp*sin(2*M_PI*sinefreq*(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat + sinephase);
		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////DM Variations////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


       if(((MNStruct *)globalcontext)->incDM==2){

			for (int i=0; i<FitDMCoeff/2; i++){
				int pnum=pcount;
				double pc=Cube[pcount];
				freqs[startpos+i]=((MNStruct *)globalcontext)->sampleFreq[startpos/2 - ((MNStruct *)globalcontext)->incFloatRed+i]/maxtspan;
				freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];
	
				powercoeff[startpos+i]=pow(10.0,pc)/(maxtspan*24*60*60);
				powercoeff[startpos+i+FitDMCoeff/2]=powercoeff[startpos+i];
				freqdet=freqdet+2*log(powercoeff[startpos+i]);
				pcount++;
			}
           	 


                for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                        DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                }
                
            for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

        for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i+FitDMCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

        }
        else if(((MNStruct *)globalcontext)->incDM==5){


                for(int i = 0; i < FitDMCoeff; i++){
                        SignalCoeff[startpos + i] = Cube[pcount];
                        pcount++;
                }

		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitDMPL; pl ++){
			double DMamp=Cube[pcount];
			pcount++;
			double DMindex=Cube[pcount];
			pcount++;

			
   			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
    			

			DMamp=pow(10.0, DMamp);
			if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(DMamp); }
			for (int i=0; i<FitDMCoeff/2; i++){
	
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[startpos/2 - ((MNStruct *)globalcontext)->incFloatRed +i]/maxtspan;
				freqs[startpos+i+FitDMCoeff/2]=freqs[startpos+i];
				
 				double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-DMindex))/(maxtspan*24*60*60);	
 				powercoeff[startpos+i]+=rho;
 				powercoeff[startpos+i+FitDMCoeff/2]+=rho;
			}
		}
		
		
		int coefftovary=0;
		double amptovary=0.0;
		if(((MNStruct *)globalcontext)->varyDMCoeff==1){
			coefftovary=int(pow(10.0,Cube[pcount]))-1;
			pcount++;
			amptovary=pow(10.0,Cube[pcount])/(maxtspan*24*60*60);
			pcount++;

			powercoeff[startpos+coefftovary]=amptovary;
			powercoeff[startpos+coefftovary+FitDMCoeff/2]=amptovary;	
		}	
			
		
		for (int i=0; i<FitDMCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
        }

        for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

        for(int i=0;i<FitDMCoeff/2;i++){
                for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
                        FMatrix[k][startpos+i+FitDMCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
                }
        }

	startpos+=FitDMCoeff;

    }
    

    


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////DM Power Spectrum Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	
	if(((MNStruct *)globalcontext)->incDMEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMEvent; i++){
                        double DMamp=DMEventInfo[i][2];
                        double DMindex=DMEventInfo[i][3];
			double DMOff=DMEventInfo[i][4];
			double DMLinear=0;//DMEventInfo[i][5];
			double DMQuad=0;//DMEventInfo[i][6];

                        double Tspan = DMEventInfo[i][1];
                        double f1yr = 1.0/3.16e7;
			int DMEventnumCoeff=int(Tspan/averageTSamp);

                        for (int c=0; c<DMEventnumCoeff; c++){
                                freqs[startpos+c]=((double)(c+1))/Tspan;
                                freqs[startpos+c+DMEventnumCoeff]=freqs[startpos+c];

                                double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freqs[startpos+c]*365.25,(-DMindex))/(maxtspan*24*60*60);
                                powercoeff[startpos+c]+=rho;
                                powercoeff[startpos+c+DMEventnumCoeff]+=rho;
				freqdet=freqdet+2*log(powercoeff[startpos+c]);
                        }


			double *DMshapecoeff=new double[3];
			double *DMshapeVec=new double[3];
			DMshapecoeff[0]=DMOff;
			DMshapecoeff[1]=DMLinear;			
			DMshapecoeff[2]=DMQuad;



			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;


				if(time < DMEventInfo[i][0]+Tspan && time > DMEventInfo[i][0]){

					Resvec[k] -= DMOff*DMVec[k];
					Resvec[k] -= (time-DMEventInfo[i][0])*DMLinear*DMVec[k];
					Resvec[k] -= pow((time-DMEventInfo[i][0]),2)*DMQuad*DMVec[k];

				}
			}

		 	for(int c=0;c<DMEventnumCoeff;c++){
                		for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
                        		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
					if(time < DMEventInfo[i][0]+Tspan && time > DMEventInfo[i][0]){
						FMatrix[k][startpos+c]=cos(2*M_PI*freqs[startpos+c]*time)*DMVec[k];
                        			FMatrix[k][startpos+c+DMEventnumCoeff]=sin(2*M_PI*freqs[startpos+c]*time)*DMVec[k];
					}
					else{
						FMatrix[k][startpos+c]=0;
						FMatrix[k][startpos+c+DMEventnumCoeff]=0;
					}
                		}
		        }

			startpos+=2*DMEventnumCoeff;



		}
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  DM Shape Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	if(((MNStruct *)globalcontext)->incDMShapeEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMShapeEvent; i++){

			int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=Cube[pcount];
			pcount++;


			globalwidth=EventWidth;

			double *DMshapecoeff=new double[numDMShapeCoeff];
			double *DMshapeVec=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *DMshapeNorm=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
				double DMsignal=0;
				for(int c=0; c < numDMShapeCoeff; c++){
					DMsignal += DMshapeNorm[c]*DMshapeVec[c]*DMshapecoeff[c]*DMVec[k];
				}

				  Resvec[k] -= DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] DMshapecoeff;
		delete[] DMshapeVec;
		delete[] DMshapeNorm;

		}
	}





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  DM Scatter Shape Events///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


	if(((MNStruct *)globalcontext)->incDMScatterShapeEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMScatterShapeEvent; i++){

			int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMScatterShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=globalwidth;//Cube[pcount];
			pcount++;
                        double EventFScale=Cube[pcount];
			pcount++;

			//EventWidth=globalwidth;

			double *DMshapecoeff=new double[numDMShapeCoeff];
			double *DMshapeVec=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *DMshapeNorm=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
				double DMsignal=0;
				for(int c=0; c < numDMShapeCoeff; c++){
					DMsignal += DMshapeNorm[c]*DMshapeVec[c]*DMshapecoeff[c]*pow(DMVec[k],EventFScale/2.0);
				}

				  Resvec[k] -= DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] DMshapecoeff;
		delete[] DMshapeVec;
		delete[] DMshapeNorm;

		}
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract Yearly DM//////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


	if(((MNStruct *)globalcontext)->yearlyDM == 1){
		double yearlyamp=pow(10.0,Cube[pcount]);
		pcount++;
		double yearlyphase=Cube[pcount];
		pcount++;
		for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			Resvec[o]-= yearlyamp*sin((2*M_PI/365.25)*(double)((MNStruct *)globalcontext)->pulse->obsn[o].bat + yearlyphase)*DMVec[o];
		}
	}





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Band DM/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  


        if(((MNStruct *)globalcontext)->incDMScatter == 1 || ((MNStruct *)globalcontext)->incDMScatter == 2  || ((MNStruct *)globalcontext)->incDMScatter == 3 ){

                if(((MNStruct *)globalcontext)->incDM == 0){
                        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                                DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                        }
                }



		double startfreq=0;
		double stopfreq=0;


		if(((MNStruct *)globalcontext)->incDMScatter == 1){		
			startfreq = 0;
			stopfreq=1000;
		}
		else  if(((MNStruct *)globalcontext)->incDMScatter == 2){
	                startfreq = 1000;
	                stopfreq=1800;
		}
	        else if(((MNStruct *)globalcontext)->incDMScatter == 3){
	                startfreq = 1800;
	                stopfreq=10000;
	        }



		double DMamp=Cube[pcount];
		pcount++;
		double DMindex=Cube[pcount];
		pcount++;

		double Tspan = maxtspan;
		double f1yr = 1.0/3.16e7;

		DMamp=pow(10.0, DMamp);
		if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(DMamp); }
		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (DMamp*DMamp)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-DMindex))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}

		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;	
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos += FitDMScatterCoeff;

    	}


        if(((MNStruct *)globalcontext)->incDMScatter == 4 ){


                if(((MNStruct *)globalcontext)->incDM == 0){
                        for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
                                DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
                        }
                }



		double Amp1=Cube[pcount];
		pcount++;
		double index1=Cube[pcount];
		pcount++;

		double Amp2=Cube[pcount];
		pcount++;
		double index2=Cube[pcount];
		pcount++;

		double Amp3=Cube[pcount];
		pcount++;
		double index3=Cube[pcount];
		pcount++;
		
		double Tspan = maxtspan;
		double f1yr = 1.0/3.16e7;
		

		Amp1=pow(10.0, Amp1);
		Amp2=pow(10.0, Amp2);
		Amp3=pow(10.0, Amp3);

		if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(Amp1) + log(Amp2) + log(Amp3); }


		double startfreq=0;
		double stopfreq=0;

		/////////////////////////Amp1/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 0;
		stopfreq=1000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp1*Amp1)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}

		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////Amp2/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 1000;
		stopfreq=2000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp2*Amp2)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index2))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////Amp3/////////////////////////////////////////////////////////////////////////////////////////////


		startfreq = 2000;
		stopfreq=10000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp3*Amp3)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index3))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);//*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}

		startpos=startpos+FitDMScatterCoeff;
	}	


        if(((MNStruct *)globalcontext)->incDMScatter == 5 ){

	        
		if(((MNStruct *)globalcontext)->incDM == 0){
			for(int o=0;o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		       		DMVec[o]=1.0/(DMKappa*pow((double)((MNStruct *)globalcontext)->pulse->obsn[o].freqSSB,2));
	      		}
		}

		double Amp1=Cube[pcount];
		pcount++;
		double index1=Cube[pcount];
		pcount++;


		double Tspan = maxtspan;
		double f1yr = 1.0/3.16e7;
		

		Amp1=pow(10.0, Amp1);

		if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(Amp1); }


		double startfreq=0;
		double stopfreq=0;

		/////////////////////////50cm/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 0;
		stopfreq=1000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp1*Amp1)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}

		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////20cm/////////////////////////////////////////////////////////////////////////////////////////////

		startfreq = 1000;
		stopfreq=1800;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp1*Amp1)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}


		startpos=startpos+FitDMScatterCoeff;

		/////////////////////////10cm/////////////////////////////////////////////////////////////////////////////////////////////


		startfreq = 1800;
		stopfreq=10000;

		for (int i=0; i<FitDMScatterCoeff/2; i++){

			freqs[startpos+i]=((double)(i+1.0))/maxtspan;
			freqs[startpos+i+FitDMScatterCoeff/2]=freqs[startpos+i];
			
			double rho = (Amp1*Amp1)*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-index1))/(maxtspan*24*60*60);	
			powercoeff[startpos+i]+=rho;
			powercoeff[startpos+i+FitDMScatterCoeff/2]+=rho;
		}
		
		
			
		for (int i=0; i<FitDMScatterCoeff/2; i++){
			freqdet=freqdet+2*log(powercoeff[startpos+i]);
		}


		for(int i=0;i<FitDMScatterCoeff/2;i++){
		        for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				if(((MNStruct *)globalcontext)->pulse->obsn[k].freq > startfreq && ((MNStruct *)globalcontext)->pulse->obsn[k].freq < stopfreq){
		               		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
		                	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time)*DMVec[k];
				}
				else{	
					FMatrix[k][startpos+i]=0;
					FMatrix[k][startpos+i+FitDMScatterCoeff/2]=0;
				}
		        }
		}

		startpos=startpos+FitDMScatterCoeff;
	}	





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Add Group Noise/////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

        if(((MNStruct *)globalcontext)->incGroupNoise > 0){

		for(int g = 0; g < ((MNStruct *)globalcontext)->incGroupNoise; g++){

			int GrouptoFit=0;
			double GroupStartTime = 0;
			double GroupStopTime = 0;
			if(((MNStruct *)globalcontext)->FitForGroup[g][0] == -1){
				GrouptoFit = floor(Cube[pcount]);
				pcount++;
			}
			else{
				GrouptoFit = ((MNStruct *)globalcontext)->FitForGroup[g][0];
				
			}

                        if(((MNStruct *)globalcontext)->FitForGroup[g][1] == 1){
                                GroupStartTime = Cube[pcount];
                                pcount++;
				GroupStopTime = Cube[pcount];
                                pcount++;

                        }
                        else{
                                GroupStartTime = 0;
				GroupStopTime = 10000000.0;

                        }

		

			double GroupAmp=Cube[pcount];
			pcount++;
			double GroupIndex=Cube[pcount];
			pcount++;


		
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
		

			GroupAmp=pow(10.0, GroupAmp);
			if(((MNStruct *)globalcontext)->DMPriorType ==1) { uniformpriorterm += log(GroupAmp); }

			for (int i=0; i<FitGroupNoiseCoeff/2; i++){

				freqs[startpos+i]=((double)(i+1.0))/maxtspan;
				freqs[startpos+i+FitGroupNoiseCoeff/2]=freqs[startpos+i];
			
				double rho = (GroupAmp*GroupAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[startpos+i]*365.25,(-GroupIndex))/(maxtspan*24*60*60);	
				powercoeff[startpos+i]+=rho;
				powercoeff[startpos+i+FitGroupNoiseCoeff/2]+=rho;
			}
		
		
			
			for (int i=0; i<FitGroupNoiseCoeff/2; i++){
				freqdet=freqdet+2*log(powercoeff[startpos+i]);
			}


			for(int i=0;i<FitGroupNoiseCoeff/2;i++){
				for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
					//printf("Groups %i %i %i %i\n", i,k,GrouptoFit,((MNStruct *)globalcontext)->sysFlags[k]);
				if(((MNStruct *)globalcontext)->GroupNoiseFlags[k] == GrouptoFit && (double)((MNStruct *)globalcontext)->pulse->obsn[k].bat > GroupStartTime && (double)((MNStruct *)globalcontext)->pulse->obsn[k].bat < GroupStopTime){
				//	if(((MNStruct *)globalcontext)->GroupNoiseFlags[k] == GrouptoFit){
				       		double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				        	FMatrix[k][startpos+i]=cos(2*M_PI*freqs[startpos+i]*time);
						FMatrix[k][startpos+i+FitGroupNoiseCoeff/2]=sin(2*M_PI*freqs[startpos+i]*time);
					}
					else{	
						FMatrix[k][startpos+i]=0;
						FMatrix[k][startpos+i+FitGroupNoiseCoeff/2]=0;
					}
				}
			}



			startpos=startpos+FitGroupNoiseCoeff;
		}

    }


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Add ECORR Coeffs////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	if(((MNStruct *)globalcontext)->incNGJitter >0){
		for(int i =0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
			powercoeff[startpos+i] = ECORRPrior[i];
			freqdet = freqdet + log(ECORRPrior[i]);
		}

		for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
			for(int i=0; i < ((MNStruct *)globalcontext)->numNGJitterEpochs; i++){
				FMatrix[k][startpos+i] = ((MNStruct *)globalcontext)->NGJitterMatrix[k][i];
			}
		}

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Make stochastic signal//////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double *SignalVec = new double[((MNStruct *)globalcontext)->pulse->nobs];

	
	dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->pulse->nobs,totCoeff,'N');

	for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){


		Resvec[o] = Resvec[o] - SignalVec[o];
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Time domain likelihood//////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 

 
/*	static unsigned int oldcw;
	if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
		fpu_fix_start(&oldcw);
	}
	
*/

	double tdet=0;
	double timelike=0;

	for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
		timelike+=Resvec[o]*Resvec[o]*Noise[o];
		tdet -= log(Noise[o]);
	}
/*	dd_real ddtimelike=0.0;
	if(((MNStruct *)globalcontext)->uselongdouble ==1){
		for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){
			dd_real res = (dd_real)Resvec[o];
			dd_real Nval = (dd_real)Noise[o];
			dd_real chicomp = res*res*Nval;
			ddtimelike += chicomp;
		}
	}

	
        qd_real qdtimelike=0.0;
        if(((MNStruct *)globalcontext)->uselongdouble ==2){
                for(int o=0; o<((MNStruct *)globalcontext)->pulse->nobs; o++){

                        qd_real res = (qd_real)Resvec[o];
                        qd_real Nval = (qd_real)Noise[o];
                        qd_real chicomp = res*res*Nval;

                        qdtimelike+=chicomp;
                }
        }

*/
/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Fourier domain likelihood///////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 


	double freqlike = 0;
	for(int i = 0; i < totCoeff; i++){
		freqlike += SignalCoeff[i]*SignalCoeff[i]/powercoeff[i];
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Get Final likelihood////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////// 



	double lnew=-0.5*(tdet+freqdet+timelike+freqlike) + uniformpriorterm;
	//printf("like, %g %g %g %g %g %g \n", lnew, tdet, freqdet, timelike, freqlike, uniformpriorterm);

	if(isnan(lnew) || isinf(lnew)){

		lnew=-pow(10.0,20);
		
	}



/*        if(((MNStruct *)globalcontext)->uselongdouble > 0 ){
                fpu_fix_end(&oldcw);
        }

*/

	delete[] DMVec;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] powercoeff;
	delete[] freqs;
	delete[] Noise;
	delete[] Resvec;
	delete[] SignalCoeff;
	delete[] SignalVec;
	
	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[]FMatrix[j];
	}
	delete[]FMatrix;

	if(((MNStruct *)globalcontext)->incNGJitter >0){
		delete[] ECORRPrior;
	}

	//printf("CPUChisq: %g %g %g %g %g %g \n",lnew,jointdet,tdet,freqdet,timelike,freqlike);
	return lnew;


}

double  AllTOAJitterLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){



	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
				//printf("Jump: %i %i %i %.25Lg \n", p, k, l, JumpVec[p]);
			}
                 }
           }

	}


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		//if(p==0){printf("before: %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[p].origsat, ((MNStruct *)globalcontext)->pulse->obsn[p].sat, ((MNStruct *)globalcontext)->pulse->obsn[p].bat);}
		//printf("sat: %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[p].origsat, ((MNStruct *)globalcontext)->pulse->obsn[p].sat, phase);
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;

	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       /* Form residuals */

	  
	//for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
	//	printf("res: %i %.25Lg %.25Lg \n", p, ((MNStruct *)globalcontext)->pulse->obsn[p].bat, ((MNStruct *)globalcontext)->pulse->obsn[p].residual);  
	//} 
	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }
	  
	  
	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr  - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;

	double shapecoeff[numcoeff];

	for(int i =0; i < numcoeff; i++){

		shapecoeff[i]= Cube[pcount];
		Cube[pcount] = shapecoeff[i];
		pcount++;
	}

	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;

	//printf("beta %g %g %g \n", Cube[pcount], beta, double(((MNStruct *)globalcontext)->ReferencePeriod));

	pcount++;
	
	double lnew = 0;
	int badshape = 0;
	
	 for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	    int nTOA = t;
	    
	    double detN  = 0;
	    double Chisq  = 0;
	    double logMargindet = 0;
	    double Marginlike = 0;	 
	    double JitterPrior = 0;	   
 
	    double profilelike=0;
	    
	    long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
	    long double FoldingPeriodDays = FoldingPeriod/SECDAY;
	    int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
	    double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
	    double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
	    long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

	    double *Betatimes = new double[Nbins];
	    double **HermiteJitterpoly =  new double*[Nbins];
	    double **JitterBasis  =  new double*[Nbins];
	    double **Hermitepoly =  new double*[Nbins];
	    for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
            for(int i =0;i<Nbins;i++){HermiteJitterpoly[i]=new double[numcoeff+1];for(int j =0;j<numcoeff+1;j++){HermiteJitterpoly[i][j]=0;}}
	    for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){JitterBasis[i][j]=0;}}
	
	



	

	    double sigma=noiseval;
	    sigma=pow(10.0, sigma)*sqrt(double(Nbins)/Tobs)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

	    long double binpos = ModelBats[nTOA];
	    if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

	    long double minpos = binpos - FoldingPeriodDays/2;
	    if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
	    long double maxpos = binpos + FoldingPeriodDays/2;
	    if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    
	    for(int j =0; j < Nbins; j++){
		    long double timediff = 0;
		    long double bintime = ProfileBats[t][j];
		    if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
		    }
		    else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
		    }
		    else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
		    }

		    timediff=timediff*SECDAY;


		    Betatimes[j]=timediff/beta;
		    TNothpl(numcoeff+1,Betatimes[j],HermiteJitterpoly[j]);
		    for(int k =0; k < numcoeff+1; k++){
			    double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
			    HermiteJitterpoly[j][k]=HermiteJitterpoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
			    if(k<numcoeff){ Hermitepoly[j][k] = HermiteJitterpoly[j][k];}
			
		    }

			JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*HermiteJitterpoly[j][1]);
		   for(int k =1; k < numcoeff; k++){
			
			JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*HermiteJitterpoly[j][k-1] - sqrt(double(k+1))*HermiteJitterpoly[j][k+1]);
		   }	
	    }


	    double *shapevec  = new double[Nbins];
	    double *Jittervec = new double[Nbins];
	
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
	    dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');

	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    double modelflux=0;
	    double testmodelflux=0;	
            double dataflux = 0;
	//	printf("beta is %g \n", beta);

	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);
	           // printf("%i %g \n", j, shapevec[j]);
		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}


		    testmodelflux += shapevec[j]*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
                //    printf("%g %g %g \n", double(ProfileBats[t][1] - ProfileBats[t][0]), double(beta), double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24);
	    }
	    for(int j =0; j < numcoeff; j=j+2){
		modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Factorials[j]/(((MNStruct *)globalcontext)->Factorials[j - j/2]*((MNStruct *)globalcontext)->Factorials[j/2]))*shapecoeff[j];
	   }
	   //printf("fluxes %g %g \n", modelflux, testmodelflux);
	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////
	

	    double **M = new double*[Nbins];
	    double **NM = new double*[Nbins];

	    for(int i =0; i < Nbins; i++){
		    M[i] = new double[3];
		    NM[i] = new double[3];
		    
		    M[i][0] = 1;
		    M[i][1] = shapevec[i];
		    M[i][2] = Jittervec[i];

		    NM[i][0] = M[i][0]/(sigma*sigma);
		    NM[i][1] = M[i][1]/(sigma*sigma);
		    NM[i][2] = M[i][2]/(sigma*sigma);
	    }


	    double **MNM = new double*[3];
	    for(int i =0; i < 3; i++){
		    MNM[i] = new double[3];
	    }

	    dgemm(M, NM , MNM, Nbins, 3,Nbins, 3, 'T', 'N');


	    MNM[2][2] += pow(10.0,20);

	    double *dNM = new double[3];
	    double *TempdNM = new double[3];
	    dgemv(M,NDiffVec,dNM,Nbins,3,'T');

	    for(int i =0; i < 3; i++){
		TempdNM[i] = dNM[i];
	    }


            int info=0;
	    double Margindet = 0;
            dpotrfInfo(MNM, 3, Margindet, info);
            dpotrs(MNM, TempdNM, 3);

	    
	    double maxoffset = TempdNM[0];
            double maxamp = TempdNM[1];
            double maxJitter = TempdNM[2];


	    
	    if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
	      
	      
		Chisq = 0;
		detN = 0;
	      
		double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];//sqrt(double(Nbins)/Tobs)*EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		profnoise = profnoise*profnoise;
		
		double MLSigma = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
		      double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxamp*shapevec[j] - maxoffset - maxJitter*Jittervec[j];
		      if(shapevec[j] <= maxshape*0.01){MLSigma += res*res; MLSigmaCount += 1;}
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);

		for(int j =0; j < Nbins; j++){
			if(shapevec[j] > maxshape*0.001){
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxoffset)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
					//printf("flux: %i %i %g %g \n", t,j,dataflux,(double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset);
				}
			}
		}	

		//printf("max: %i %g %g %g \n", t, maxamp, dataflux/modelflux, maxamp/(dataflux/modelflux));

		MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];


		
		for(int j =0; j < Nbins; j++){
		  
			double noise = MLSigma*MLSigma;
		
			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];

			if(shapevec[j] < 0)badshape = 1;
			NDiffVec[j] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
			
			 NM[j][0] = M[j][0]/(noise);
			 NM[j][1] = M[j][1]/(noise);
			 
			 M[j][2] = M[j][2]*dataflux/modelflux/beta;
			 NM[j][2] = M[j][2]/(noise);
		
		}
		
		
			
		    dgemm(M, NM , MNM, Nbins, 3,Nbins, 3, 'T', 'N');


		    JitterPrior = log(profnoise);
		    MNM[2][2] += 1.0/profnoise;

		    dgemv(M,NDiffVec,dNM,Nbins,3,'T');

		    for(int i =0; i < 3; i++){
			TempdNM[i] = dNM[i];
		    }


		    info=0;
		    Margindet = 0;
		    dpotrfInfo(MNM, 3, Margindet, info);
		    dpotrs(MNM, TempdNM, 3);


			
			if(((MNStruct *)globalcontext)->sysFlags[nTOA] == 4 && nTOA == 317){
			printf("Jitter vals: %i %g %g\n", nTOA, TempdNM[2], sqrt(profnoise));
			for(int j =0; j < Nbins; j++){
				printf("Jitter vals: %i %i %g %g %g %g \n", nTOA, j, TempdNM[2], TempdNM[2]*M[j][2], TempdNM[0]*M[j][0]+TempdNM[1]*M[j][1], (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]);
			}}	
	

			//printf("Jitter vals: %g %g %g %g \n", maxJitter, maxamp, maxJitter/maxamp, TempdNM[2]);
			
		}
		    
		    logMargindet = Margindet;

		    Marginlike = 0;
		    for(int i =0; i < 3; i++){
		    	Marginlike += TempdNM[i]*dNM[i];
		    }

	    profilelike = -0.5*(detN + Chisq + logMargindet + JitterPrior - Marginlike);
	    lnew += profilelike;
	//    if(nTOA == 1){printf("Like: %i  %g %g %g %g \n", nTOA,detN, Chisq, logMargindet, Marginlike);}

	    delete[] shapevec;
	    delete[] Jittervec;
	    delete[] NDiffVec;
	    delete[] dNM;
	    delete[] Betatimes;

	    for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] HermiteJitterpoly[j];
		    delete[] M[j];
		    delete[] NM[j];
	    }
	    delete[] Hermitepoly;
	    delete[] HermiteJitterpoly;
	    delete[] JitterBasis;
	    delete[] M;
	    delete[] NM;

	    for (int j = 0; j < 3; j++){
		    delete[] MNM[j];
	    }
	    delete[] MNM;
	
	
	 }
	 
	 
	 
	 

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}

	//printf("Like: %.10g \n", lnew);
	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;


	return lnew;

}

double  AllTOAStocProfLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	//printf("In like \n");

	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
	//	printf("timing param %i %.25Lg	\n", p, LDparams[pcount]);
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;

	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       /* Form residuals */




/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  

	//printf("End of White\n");



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Red Noise///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }

	double maxtspan=1*(end-start);


	
	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int totCoeff=0;
	if(((MNStruct *)globalcontext)->incRED != 0)totCoeff+=FitRedCoeff;



	double FreqDet = 0;

	double *freqs = new double[totCoeff];
   	double *SignalCoeff = new double[totCoeff];

	for(int i = 0; i < totCoeff; i++){
		SignalCoeff[i] = 0;
	}	


	double *powercoeff=new double[totCoeff];
	for(int o=0;o<totCoeff; o++){
		powercoeff[o]=0;
	}
	
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			FMatrix[i][j]=0;
		}
	}


	int startpos=0;
	if(((MNStruct *)globalcontext)->incRED==5){

		FreqDet=0;

		for(int i = 0; i < FitRedCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			pcount++;
		}
			
		
		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitRedPL; pl ++){

			double Tspan = maxtspan;
                        double f1yr = 1.0/3.16e7;


			double redamp=Cube[pcount];
			pcount++;
			double redindex=Cube[pcount];
			pcount++;

			
			redamp=pow(10.0, redamp);
			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm +=log(redamp); }



			double Agw=redamp;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
				freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];
				
				double rho=0;

 				rho = (Agw*Agw/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-redindex))/(maxtspan*24*60*60);

 				powercoeff[i]+= rho;
 				powercoeff[i+FitRedCoeff/2]+= rho;

				

			}
		}
		
		
		double GWBAmp=0;
		if(((MNStruct *)globalcontext)->incGWB==1){
			GWBAmp=pow(10.0,Cube[pcount]);
			pcount++;

			uniformpriorterm += log(GWBAmp);
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				double rho = (GWBAmp*GWBAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-4.333))/(maxtspan*24*60*60);
				powercoeff[i]+= rho;
				powercoeff[i+FitRedCoeff/2]+= rho;
			}
		}

		for (int i=0; i<FitRedCoeff/2; i++){
			FreqDet=FreqDet+2*log(powercoeff[i]);
		}
		
		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);

			}
		}

		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
			}
		}

		startpos=FitRedCoeff;

    	}

	double *SignalVec = new double[((MNStruct *)globalcontext)->pulse->nobs];

	
	dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->pulse->nobs,totCoeff,'N');

	double FreqLike = 0;
	for(int i = 0; i < totCoeff; i++){
		FreqLike += SignalCoeff[i]*SignalCoeff[i]/powercoeff[i];
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr  - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;

	double shapecoeff[numcoeff];
	double StocProfCoeffs[numshapestoccoeff];

	for(int i =0; i < numcoeff; i++){
		if(((MNStruct *)globalcontext)->FixProfile==0){
			shapecoeff[i]= Cube[pcount];
			pcount++;
		}
		else{
			shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
			//printf("Fixing mean: %i %g n", i, shapecoeff[i]);
		}
	}

	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;

	pcount++;

	for(int i =0; i < numshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		//uniformpriorterm += log(pow(10.0, Cube[pcount]));
		pcount++;
	}

	if(numcoeff+1>=numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double lnew = 0;
	int badshape = 0;

	int minoffpulse=750;
	int maxoffpulse=1250;
       // int minoffpulse=500;
       // int maxoffpulse=1500;
	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	//	printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 
		double JitterPrior = 0;	   

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **AllHermiteBasis =  new double*[Nbins];
		double **JitterBasis  =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
		for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){JitterBasis[i][j]=0;}}
	
	



	

		double sigma=noiseval;
		sigma=pow(10.0, sigma);
//		printf("noise details %i %g %g %g \n", t, noiseval, Tobs, EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]]);

		long double binpos = ModelBats[nTOA];

		if(((MNStruct *)globalcontext)->incRED==5){
			long double rednoiseshift = (long double) SignalVec[t];
			binpos+=rednoiseshift/SECDAY;
			//printf("Red noise shift %i %Lg \n", t, rednoiseshift);
		}

		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    
		for(int j =0; j < Nbins; j++){
			long double timediff = 0;
			long double bintime = ProfileBats[t][j];
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;
			//if(nTOA == 50){printf("timediff %i %g %g\n", j, (double)timediff, beta);}

			Betatimes[j]=timediff/beta;
			TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

			for(int k =0; k < maxshapecoeff; k++){
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));//*((MNStruct *)globalcontext)->Factorials[k]);
				AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

				if(k<numcoeff){ Hermitepoly[j][k] = AllHermiteBasis[j][k];}
			
			}

			JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1]);
			for(int k =1; k < numcoeff; k++){
				JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1] - sqrt(double(k+1))*AllHermiteBasis[j][k+1]);
			}	
	    }


	    double *shapevec  = new double[Nbins];
	    double *Jittervec = new double[Nbins];
	
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
    	    dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');
	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    double modelflux=0;
	    double testmodelflux=0;	
            double dataflux = 0;


	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);

		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}


		    testmodelflux += shapevec[j]*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;

	    }
	    for(int j =0; j < numcoeff; j=j+2){
		//printf("modelflux: %i %g\n", j, ((MNStruct *)globalcontext)->Binomial[j]);
		modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[j];
	   }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];
		double **NM = new double*[Nbins];

		int Msize = 2+numshapestoccoeff;
		int Mcounter=0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			Msize++;
		}


		for(int i =0; i < Nbins; i++){
			Mcounter=0;
			M[i] = new double[Msize];
			NM[i] = new double[Msize];

			M[i][0] = 1;
			M[i][1] = shapevec[i];

			Mcounter = 2;

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
//				printf("%i %i %g %g %g \n", nTOA, i, M[i][0], M[i][1], M[i][2]);
				M[i][Mcounter] = Jittervec[i];
				Mcounter++;
			}			



			for(int j = 0; j < numshapestoccoeff; j++){

			    M[i][Mcounter+j] = AllHermiteBasis[i][j];

			}

			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(sigma*sigma);
			//	if(nTOA==50){printf("M %i %i %g %g \n",i,j,M[i][j], sigma); }
			}
		  
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');
		//printf("MNM %g %g %g %g \n", MNM[0][0], MNM[0][1], MNM[1][0], MNM[1][1]);

		for(int j = 2; j < Msize; j++){
			//printf("before: %i %g \n", j, MNM[j][j]);
			MNM[j][j] += pow(10.0,20);
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


		double maxoffset = TempdNM[0];
		double maxamp = TempdNM[1];


		//printf("max %i %g %g %g\n", t, maxoffset, maxamp, Margindet);

		      
		Chisq = 0;
		detN = 0;

		double noisemean=0;
		for(int j =minoffpulse; j < maxoffpulse; j++){
			noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
		}

		noisemean = noisemean/(maxoffpulse-minoffpulse);

		double MLSigma = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
		      double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxamp*shapevec[j] - maxoffset;

			res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] -noisemean;
//		      if(shapevec[j] <= maxshape*0.001){MLSigma += res*res; MLSigmaCount += 1;}
		      if(j > minoffpulse && j < maxoffpulse){
				MLSigma += res*res; MLSigmaCount += 1;
		      }
//		      
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);
		double tempdataflux=0;
		for(int j =0; j < Nbins; j++){
			if(shapevec[j] > maxshape*0.001){
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxoffset)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
					tempdataflux += ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset);
			//		if(t==1){
                          //                      printf("dataflux %i %g %g %g %g %g\n", j, maxoffset, ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]), dataflux, tempdataflux, double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24);
                            //            }

				}
			}
		}	

 

//		MLSigma = MLSigma*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

//		double highfreqnoise = sqrt(double(Nbins)/Tobs)*SQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double highfreqnoise = SQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double flatnoise = EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

//		double *testflux=new double[numshapestoccoeff];
//		for(int j = 0; j < numshapestoccoeff; j++){
//			testflux[j] = 0;
//		}


		for(int i =0; i < Nbins; i++){
		  	Mcounter=2;
			double noise = MLSigma*MLSigma +  pow(highfreqnoise*maxamp*shapevec[i],2);

			if(shapevec[i] > maxshape*0.001){
				noise +=  pow(flatnoise*maxamp*maxshape,2);
			}
			//printf("detN: %i %i %g %g %g %g\n", nTOA, i, MLSigma, highfreqnoise, maxamp, noise);
			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];

			if(shapevec[i] < 0)badshape = 1;
			NDiffVec[i] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
	
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				//printf("EQUAD: %i %g %g %g %g \n", i, dataflux, modelflux, beta, M[i][2]);
				M[i][2] = M[i][2]*dataflux/modelflux/beta;
				Mcounter++;
			}
			
		   	for(int j = 0; j < numshapestoccoeff; j++){
//				testflux[j] += pow(M[i][Mcounter+j]*sqrt(double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24)*dataflux, 2);
//				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux*sqrt(Tobs/double(Nbins));
				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux;

		    	}


			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(noise);
			}


		}
		
//		for(int j = 0; j < numshapestoccoeff; j++){
//			printf("test flux %i %i %g %g \n", t, j, testflux[j], pow(dataflux,2));
//		}
//		delete[] testflux;
			
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


		Mcounter=2;
		double JitterDet = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];//sqrt(double(Nbins)/Tobs)*EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			profnoise = profnoise*profnoise;
		    	JitterDet = log(profnoise);
			//printf("after: %i %g %g\n", Mcounter, MNM[2][2], 1.0/profnoise);
		    	MNM[2][2] += 1.0/profnoise;
			Mcounter++;
		}


		double StocProfDet = 0;
		for(int j = 0; j < numshapestoccoeff; j++){
			if(((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat > 57075.0){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[Mcounter+j][Mcounter+j] +=  1.0/StocProfCoeffs[j];
			}
			else{
                        StocProfDet += log(pow(10.0, -20));
                        MNM[Mcounter+j][Mcounter+j] +=  1.0/pow(10.0, -20);
			}
		}

		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		info=0;
		Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);



//		if(t==1){
//			double *tempsignal = new double[Nbins];
//			dgemv(M,TempdNM,tempsignal,Nbins,Msize,'N');
	//		for(int j = 0; j < numshapestoccoeff; j++){
	//			double stocflux = 0;
	//			for(int k = 0; k < Nbins; k++){
	//				stocflux += pow(TempdNM[2+j]*M[k][2+j],2);
	//			}
	//			printf("%i %g %g %g %g %g\n", j, StocProfCoeffs[j], TempdNM[2+j], stocflux, dataflux*dataflux/(double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24), double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24);
	//		}
			
			
//			for(int j = 0; j < Nbins; j++){
				//double justprofile = TempdNM[0]*M[j][0] + TempdNM[1]*M[j][1];
//				printf("%i %g %g\n", j,(double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1],tempsignal[j]); 
				//printf("%i %g %g %g %g %g %g %g %g %g %g %g %g \n",j, TempdNM[2]*M[j][2], TempdNM[3]*M[j][3], TempdNM[4]*M[j][4], TempdNM[5]*M[j][5], TempdNM[6]*M[j][6], TempdNM[7]*M[j][7], TempdNM[8]*M[j][8], TempdNM[9]*M[j][9], TempdNM[10]*M[j][10],TempdNM[11]*M[j][11],justprofile, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] );
//			}
//			delete[] tempsignal;
//		}			
		
			    
		logMargindet = Margindet;

		Marginlike = 0;
		for(int i =0; i < Msize; i++){
			Marginlike += TempdNM[i]*dNM[i];
		}

		profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
		lnew += profilelike;
//		printf("Like: %i %.15g %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

		delete[] shapevec;
		delete[] Jittervec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] AllHermiteBasis[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] AllHermiteBasis;
		delete[] JitterBasis;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] powercoeff;
	delete[] freqs;
	delete[] SignalCoeff;
	delete[] SignalVec;
	
	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[]FMatrix[j];
	}
	delete[]FMatrix;


	lnew += -0.5*(FreqDet + FreqLike) + uniformpriorterm;
	printf("End Like: %.10g %.10g %.10g %.10g\n", lnew, FreqDet, FreqLike, uniformpriorterm);

	return lnew;

}


double  AllTOALike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){



	double *EFAC;
	double *EQUAD;
	//printf("In one toa like\n");
        long double LDparams[ndim];
        int pcount;

//	for(int i = 0; i < ndim; i++){
//		printf("Cube: %i %g \n", i, Cube[i]);
//	}
	
	
	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){


		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

		//printf("LDparams: %i %.15Lg %.15Lg \n", p,((MNStruct *)globalcontext)->LDpriors[p][0], ((MNStruct *)globalcontext)->LDpriors[p][1]); 


	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	//printf("Phase %.15Lg,  %.15Lg,  %.15Lg \n", phase, LDparams[0], ((MNStruct *)globalcontext)->ReferencePeriod/SECDAY);
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}


	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;



	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       /* Form residuals */
	  
	  
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		//if(p==0){printf("after: %.25Lg %.25Lg %.15g \n", ((MNStruct *)globalcontext)->pulse->obsn[p].sat, ((MNStruct *)globalcontext)->pulse->obsn[p].bat, (double)((MNStruct *)globalcontext)->pulse->obsn[p].residual);}
		//printf("sat: %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[p].origsat, ((MNStruct *)globalcontext)->pulse->obsn[p].sat, phase);
	}
	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }
	  
	  
	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      //long double sat2bat = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - ((MNStruct *)globalcontext)->pulse->obsn[i].sat;
	      //printf("Sat2Bat %i %.25Lg  %.25Lg \n", i, ((MNStruct *)globalcontext)->pulse->obsn[i].sat, sat2bat/SECDAY);
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
// 		    printf("Sat2Bat %i %i %.25Lg %.25Lg %.25Lg \n", i, j, sat2bat/SECDAY, ((MNStruct *)globalcontext)->ProfileData[i][j][0], ProfileBats[i][j]);
	      }
	      
	      //ModelBats[i] = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - phase ;//- ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY - phase;
	      //printf("Bats %i %.20Lg %.20Lg %.20Lg %.20Lg \n", i,ModelBats[i],((MNStruct *)globalcontext)->ProfileInfo[i][5], ((MNStruct *)globalcontext)->pulse->obsn[i].residual, ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr);
	     // printf("Sat2BatConmp %.10Lg %.10Lg \n",((MNStruct *)globalcontext)->ProfileInfo[i][6], ((MNStruct *)globalcontext)->pulse->obsn[i].bat);
	 }


	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;

	double shapecoeff[numcoeff];

	for(int i =0; i < numcoeff; i++){

		shapecoeff[i]= Cube[pcount];
		Cube[pcount] = shapecoeff[i];
		pcount++;
	}
	//printf("Beta: %g %.25Lg \n", Cube[pcount], ((MNStruct *)globalcontext)->ReferencePeriod);
	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
	pcount++;

	double lnew = 0;
	int badshape = 0;
	
	 for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	    int nTOA = t;
	    
	    double detN  = 0;
	    double Chisq  = 0;
	    double logMargindet = 0;
	    double Marginlike = 0;	 
	    
	    double profilelike=0;
	    
	    long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
	    long double FoldingPeriodDays = FoldingPeriod/SECDAY;
	    int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
	    double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
	    double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
	    long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

	    double *Betatimes = new double[Nbins];
	    double **Hermitepoly =  new double*[Nbins];
	    for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
	
	



	

	    double sigma=noiseval;
	    sigma=pow(10.0, sigma)*sqrt(double(Nbins)/Tobs)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

	    //printf("CheckBat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1],((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat);
	    //printf("CheckSat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA,((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0],((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat );
	    long double binpos = ModelBats[nTOA];
	    if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;
	    long double minpos = binpos - FoldingPeriodDays/2;
	    //if(t==120){printf("minpos maxpos %i %.25Lg %.25Lg %.25Lg %.25Lg %.25Lg\n", t, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1], binpos, minpos, binpos + FoldingPeriodDays/2);}
	    if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
	    long double maxpos = binpos + FoldingPeriodDays/2;
	    if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];

	    //printf("Obvious time check: %.25Lg %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0]+FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0]); 

	    for(int j =0; j < Nbins; j++){
		    long double timediff = 0;
		    long double bintime = ProfileBats[t][j];
		    if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
		    }
		    else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
		    }
		    else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
		    }

		    timediff=timediff*SECDAY;
	//	    timediff = timediff - 1.6674E-7 + 6.8229E-8 + 7.8479E-8;
	//	    if(t==120){printf("Bins: %i %i %.20Lg %.20Lg %.20Lg %.20Lg %.20Lg  \n", nTOA, j, minpos, maxpos, binpos, bintime, timediff);}
			if(j == 0){
                    //            printf("Model Bat: %i %s %g %g \n", t, ((MNStruct *)globalcontext)->pulse->obsn[t].fname , (double)ProfileBats[t][j], (double)timediff);
                        }


		    Betatimes[j]=timediff/beta;
		    TNothpl(numcoeff,Betatimes[j],Hermitepoly[j]);
		    for(int k =0; k < numcoeff; k++){
			    double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
			
		//		printf("%i %i  %i %g %g %g %.25Lg %g\n", t, j, k, Hermitepoly[j][k], Bconst, Betatimes[j], timediff, beta);

			    Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
		    }
	    }


	    double *shapevec  = new double[Nbins];
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');


	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);
	            //printf("%i %i %g  %g \n", t, j, datadiff, shapevec[j]);
		    Chisq += datadiff*datadiff/(sigma*sigma);
		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
	    }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////
	

	    double **M = new double*[Nbins];
	    double **NM = new double*[Nbins];

	    for(int i =0; i < Nbins; i++){
		    M[i] = new double[2];
		    NM[i] = new double[2];
		    
		    M[i][0] = 1;
		    M[i][1] = shapevec[i];

		    NM[i][0] = M[i][0]/(sigma*sigma);
		    NM[i][1] = M[i][1]/(sigma*sigma);
	    }


	    double **MNM = new double*[2];
	    for(int i =0; i < 2; i++){
		    MNM[i] = new double[2];
	    }

	    dgemm(M, NM , MNM, Nbins, 2,Nbins, 2, 'T', 'N');

	    double *dNM = new double[2];
	    dNM[0] = 0;
	    dNM[1] = 0;

	    for(int j =0; j < Nbins; j++){
		    dNM[0] += NDiffVec[j]*M[j][0];
		    dNM[1] += NDiffVec[j]*M[j][1];
	    }
	    
	    double Margindet = MNM[0][0]*MNM[1][1] - MNM[0][1]*MNM[1][0];

            double maxamp = (1.0/Margindet)*(-dNM[0]*MNM[0][1] + dNM[1]*MNM[0][0]);
            double maxoffset = (1.0/Margindet)*(dNM[0]*MNM[1][1] - dNM[1]*MNM[0][1]);
	    
//	    if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
	      
	      
		Chisq = 0;
		detN = 0;
	      
		double profnoise = sqrt(double(Nbins)/Tobs)*EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		//printf("%i %g %g \n", nTOA, sigma, profnoise*maxamp);
		
		double MLSigma = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
		      double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxamp*shapevec[j] - maxoffset;
		      if(shapevec[j] <= maxshape*0.01){MLSigma += res*res; MLSigmaCount += 1;}
//			if(t==120){printf("res: %i %i %g %g %g %g \n", t, j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1],  maxamp*shapevec[j], maxoffset, maxamp*shapevec[j] + maxoffset);}
		}
		MLSigma=sqrt(MLSigma/MLSigmaCount)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];
	        //MLSigma=0.1;	
		for(int j =0; j < Nbins; j++){
		  
			double noise = MLSigma*MLSigma + pow(profnoise*maxamp*shapevec[j],2);
		
			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

			if(shapevec[j] < 0)badshape = 1;
			NDiffVec[j] = datadiff/(noise);
//		        printf("%i %i %g %g\n", t, j, maxamp*shapevec[j] + maxoffset, datadiff);
			Chisq += datadiff*datadiff/(noise);
			
			 NM[j][0] = M[j][0]/(noise);
			 NM[j][1] = M[j][1]/(noise);

		
		}
		
		dgemm(M, NM , MNM, Nbins, 2,Nbins, 2, 'T', 'N');
		
		dNM[0] = 0;
		dNM[1] = 0;

		for(int j =0; j < Nbins; j++){
			dNM[0] += NDiffVec[j]*M[j][0];
			dNM[1] += NDiffVec[j]*M[j][1];

		}
		
		Margindet = MNM[0][0]*MNM[1][1] - MNM[0][1]*MNM[1][0];
		
	    maxamp = (1.0/Margindet)*(-dNM[0]*MNM[0][1] + dNM[1]*MNM[0][0]);
            maxoffset = (1.0/Margindet)*(dNM[0]*MNM[1][1] - dNM[1]*MNM[0][1]);

	                for(int j =0; j < Nbins; j++){

                        if(((MNStruct *)globalcontext)->sysFlags[nTOA] == 0){
                                //printf("%i %i %g %g %g %g %g\n", nTOA, j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1], maxoffset+maxamp*shapevec[j], MLSigma, profnoise*maxamp*shapevec[j], sqrt(MLSigma*MLSigma + pow(profnoise*maxamp*shapevec[j],2)));
                        }
                }

	
//	    }
	    
	    logMargindet += log(Margindet);

	    Marginlike += (1.0/Margindet)*(dNM[0]*MNM[1][1]*dNM[0] + dNM[1]*MNM[0][0]*dNM[1] - 2*dNM[0]*MNM[0][1]*dNM[1]);

	    profilelike = -0.5*(detN + Chisq + logMargindet - Marginlike);
	    lnew += profilelike;
	///	printf("profile like %i %g %g \n", t, profilelike, lnew);
	    //if(t == 1){printf("Like: %i  %g %g %g %g \n", nTOA,detN, Chisq, logMargindet, Marginlike);}

	    delete[] shapevec;
	    delete[] NDiffVec;
	    delete[] dNM;
	    delete[] Betatimes;

	    for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] M[j];
		    delete[] NM[j];
	    }
	    delete[] Hermitepoly;
	    delete[] M;
	    delete[] NM;

	    for (int j = 0; j < 2; j++){
		    delete[] MNM[j];
	    }
	    delete[] MNM;
	
	
	 }
	 
	 
	 
	 

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}
	if(isnan(lnew)){
		lnew= -pow(10.0, 10.0);
	}
	//printf("Like: %.10g \n", lnew);
	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;


	return lnew;

}


double  AllTOASim(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){



	double *EFAC;
	double *EQUAD;
	//printf("In one toa like\n");
        long double LDparams[ndim];
        int pcount;

//	for(int i = 0; i < ndim; i++){
//		printf("Cube: %i %g \n", i, Cube[i]);
//	}
	
	
	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){


		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

		printf("LDparams: %i %.15Lg \n", p, LDparams[p]); 


	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	//printf("Phase %.15Lg,  %.15Lg,  %.15Lg \n", phase, LDparams[0], ((MNStruct *)globalcontext)->ReferencePeriod/SECDAY);
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}





	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;



	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       /* Form residuals */
	  
	  
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		//if(p==0){printf("after: %.25Lg %.25Lg %.15g \n", ((MNStruct *)globalcontext)->pulse->obsn[p].sat, ((MNStruct *)globalcontext)->pulse->obsn[p].bat, (double)((MNStruct *)globalcontext)->pulse->obsn[p].residual);}
		//printf("sat: %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[p].origsat, ((MNStruct *)globalcontext)->pulse->obsn[p].sat, phase);
	}
	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }
	  
	  
	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      //long double sat2bat = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - ((MNStruct *)globalcontext)->pulse->obsn[i].sat;
	      //printf("Sat2Bat %i %.25Lg  %.25Lg \n", i, ((MNStruct *)globalcontext)->pulse->obsn[i].sat, sat2bat/SECDAY);
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
// 		    printf("Sat2Bat %i %i %.25Lg %.25Lg %.25Lg \n", i, j, sat2bat/SECDAY, ((MNStruct *)globalcontext)->ProfileData[i][j][0], ProfileBats[i][j]);
	      }
	      
	      //ModelBats[i] = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - phase ;//- ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY - phase;
	      //printf("Bats %i %.20Lg %.20Lg %.20Lg %.20Lg \n", i,ModelBats[i],((MNStruct *)globalcontext)->ProfileInfo[i][5], ((MNStruct *)globalcontext)->pulse->obsn[i].residual, ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr);
	     // printf("Sat2BatConmp %.10Lg %.10Lg \n",((MNStruct *)globalcontext)->ProfileInfo[i][6], ((MNStruct *)globalcontext)->pulse->obsn[i].bat);
	 }


	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;

	double shapecoeff[numcoeff];

	for(int i =0; i < numcoeff; i++){

		shapecoeff[i]= Cube[pcount];
		Cube[pcount] = shapecoeff[i];
		pcount++;
	}
	//printf("Beta: %g %.25Lg \n", Cube[pcount], ((MNStruct *)globalcontext)->ReferencePeriod);
	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
	pcount++;

	double lnew = 0;
	int badshape = 0;
	
	 for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){

		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **Hermitepoly =  new double*[Nbins];
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}

	



	

		double sigma=noiseval;
		sigma=pow(10.0, sigma)*sqrt(double(Nbins)/Tobs)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

		//printf("CheckBat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1],((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat);
		//printf("CheckSat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA,((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0],((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat );
		long double binpos = ModelBats[nTOA];



		long equadseed = 1000;
                long double equadnoiseval = (long double)TKgaussDev(&equadseed)*pow(10.0, -6.3);
		//if(((MNStruct *)globalcontext)->sysFlags[nTOA] == 4){printf("EQUAD: %i %.25Lg %.25Lg %.25Lg %.25Lg \n",t,equadnoiseval/SECDAY,equadnoiseval, binpos, binpos+equadnoiseval/SECDAY);}
	        //if(((MNStruct *)globalcontext)->sysFlags[nTOA] == 4){binpos+=equadnoiseval/SECDAY;}



		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;
		long double minpos = binpos - FoldingPeriodDays/2;
		//if(t==120){printf("minpos maxpos %i %.25Lg %.25Lg %.25Lg %.25Lg %.25Lg\n", t, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1], binpos, minpos, binpos + FoldingPeriodDays/2);}
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];

		//printf("Obvious time check: %.25Lg %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0]+FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0]); 


		for(int j =0; j < Nbins; j++){

		    long double timediff = 0;
		    long double bintime = ProfileBats[t][j];

		    
		   
		    if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
		    }
		    else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
		    }
		    else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
		    }

		    timediff=timediff*SECDAY;
	//	    timediff = timediff - 1.6674E-7 + 6.8229E-8 + 7.8479E-8;
	//	    if(t==120){printf("Bins: %i %i %.20Lg %.20Lg %.20Lg %.20Lg %.20Lg  \n", nTOA, j, minpos, maxpos, binpos, bintime, timediff);}
			if(j == 0){
                    //            printf("Model Bat: %i %s %g %g \n", t, ((MNStruct *)globalcontext)->pulse->obsn[t].fname , (double)ProfileBats[t][j], (double)timediff);
                        }


		    Betatimes[j]=timediff/beta;
		    TNothpl(numcoeff,Betatimes[j],Hermitepoly[j]);
		    for(int k =0; k < numcoeff; k++){
			    double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
			
		//		printf("%i %i  %i %g %g %g %.25Lg %g\n", t, j, k, Hermitepoly[j][k], Bconst, Betatimes[j], timediff, beta);

			    Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
		    }
		}


	    double *shapevec  = new double[Nbins];
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');


	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);
	            //printf("%i %i %g  %g \n", t, j, datadiff, shapevec[j]);
		    Chisq += datadiff*datadiff/(sigma*sigma);
		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
	    }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////
	

	    double **M = new double*[Nbins];
	    double **NM = new double*[Nbins];

	    for(int i =0; i < Nbins; i++){
		    M[i] = new double[2];
		    NM[i] = new double[2];
		    
		    M[i][0] = 1;
		    M[i][1] = shapevec[i];

		    NM[i][0] = M[i][0]/(sigma*sigma);
		    NM[i][1] = M[i][1]/(sigma*sigma);
	    }


		double **MNM = new double*[2];
		for(int i =0; i < 2; i++){
		    MNM[i] = new double[2];
		}

		dgemm(M, NM , MNM, Nbins, 2,Nbins, 2, 'T', 'N');

		double *dNM = new double[2];
		dNM[0] = 0;
		dNM[1] = 0;

		for(int j =0; j < Nbins; j++){
		    dNM[0] += NDiffVec[j]*M[j][0];
		    dNM[1] += NDiffVec[j]*M[j][1];
		}

		double Margindet = MNM[0][0]*MNM[1][1] - MNM[0][1]*MNM[1][0];

		double maxamp = (1.0/Margindet)*(-dNM[0]*MNM[0][1] + dNM[1]*MNM[0][0]);
		double maxoffset = (1.0/Margindet)*(dNM[0]*MNM[1][1] - dNM[1]*MNM[0][1]);

		double MLSigma = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
		      double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxamp*shapevec[j] - maxoffset;
		      if(shapevec[j] <= maxshape*0.01){MLSigma += res*res; MLSigmaCount += 1;}
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);

		double dataflux = 0;
		for(int j =0; j < Nbins; j++){
			if(shapevec[j] > maxshape*0.001){
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxoffset)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
				}
			}
		}	


		std::string ProfileName =  ((MNStruct *)globalcontext)->pulse->obsn[t].fname;
		std::string fname = "/home/ltl21/scratch/Pulsars/ProfileData/J1909-10cm/profiles/"+ProfileName+".ASCII";
		std::ifstream ProfileFile;
                                
		ProfileFile.open(fname.c_str());
		

		std::string line; 
		getline(ProfileFile,line);
		
		ProfileFile.close();


		std::string fname2 = "/home/ltl21/scratch/Pulsars/ProfileData/J1909-10cm/SimNoNoiseProfiles/"+ProfileName+".ASCII";
	        std::ofstream simfile;
		simfile.open(fname2.c_str());
		simfile << line << "\n";
		for(int i = 0; i < Nbins; i++){
			long seed = 1000;
                        double noiseval = TKgaussDev(&seed)*MLSigma;
			double gaussflux = sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0));
			//printf("%i %i %g %g %g \n", t, i, dataflux, gaussflux, Hermitepoly[i][0]);
			simfile << i << " " <<  std::fixed  << std::setprecision(8) << (dataflux/gaussflux)*Hermitepoly[i][0] << "\n";
		}

		simfile.close();

		delete[] shapevec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < 2; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	 }
	 
	 
	 
	printf("made sim \n");
	//printf("Like: %.10g \n", lnew);
	usleep(5000000);
	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}

	
	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;


	return lnew;

}


double  AllTOAMaxLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){



	double *EFAC;
	double *EQUAD;
	//printf("In one toa like\n");
        long double LDparams[ndim];
        int pcount;

//	for(int i = 0; i < ndim; i++){
//		printf("Cube: %i %g \n", i, Cube[i]);
//	}
	
	
	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){


		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

		//printf("LDparams: %i %.15Lg %.15Lg \n", p,((MNStruct *)globalcontext)->LDpriors[p][0], ((MNStruct *)globalcontext)->LDpriors[p][1]); 


	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	//printf("Phase %.15Lg,  %.15Lg,  %.15Lg \n", phase, LDparams[0], ((MNStruct *)globalcontext)->ReferencePeriod/SECDAY);
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];	
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}


	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;



	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       /* Form residuals */
	  
	  
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		//if(p==0){printf("after: %.25Lg %.25Lg %.15g \n", ((MNStruct *)globalcontext)->pulse->obsn[p].sat, ((MNStruct *)globalcontext)->pulse->obsn[p].bat, (double)((MNStruct *)globalcontext)->pulse->obsn[p].residual);}
		//printf("sat: %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->pulse->obsn[p].origsat, ((MNStruct *)globalcontext)->pulse->obsn[p].sat, phase);
	}
	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=1;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }
	  
	  
	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      //long double sat2bat = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - ((MNStruct *)globalcontext)->pulse->obsn[i].sat;
	      //printf("Sat2Bat %i %.25Lg  %.25Lg \n", i, ((MNStruct *)globalcontext)->pulse->obsn[i].sat, sat2bat/SECDAY);
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
// 		    printf("Sat2Bat %i %i %.25Lg %.25Lg %.25Lg \n", i, j, sat2bat/SECDAY, ((MNStruct *)globalcontext)->ProfileData[i][j][0], ProfileBats[i][j]);
	      }
	      
	      //ModelBats[i] = ((MNStruct *)globalcontext)->pulse->obsn[i].bat - phase ;//- ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY - phase;
	      //printf("Bats %i %.20Lg %.20Lg %.20Lg %.20Lg \n", i,ModelBats[i],((MNStruct *)globalcontext)->ProfileInfo[i][5], ((MNStruct *)globalcontext)->pulse->obsn[i].residual, ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr);
	     // printf("Sat2BatConmp %.10Lg %.10Lg \n",((MNStruct *)globalcontext)->ProfileInfo[i][6], ((MNStruct *)globalcontext)->pulse->obsn[i].bat);
	 }


	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;

	double shapecoeff[numcoeff];

	for(int i =0; i < numcoeff; i++){

		shapecoeff[i]= Cube[pcount];
		Cube[pcount] = shapecoeff[i];
		pcount++;
	}
	//printf("Beta: %g %.25Lg \n", Cube[pcount], ((MNStruct *)globalcontext)->ReferencePeriod);
	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
	pcount++;

	double lnew = 0;
	int badshape = 0;
	
	 for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	    int nTOA = t;
	    
	    double detN  = 0;
	    double Chisq  = 0;
	    double logMargindet = 0;
	    double Marginlike = 0;	 
	    
	    double profilelike=0;
	    
	    long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
	    long double FoldingPeriodDays = FoldingPeriod/SECDAY;
	    int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
	    double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
	    double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
	    long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

	    double *Betatimes = new double[Nbins];
	    double **Hermitepoly =  new double*[Nbins];
	    for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
	
	



	

	    double sigma=noiseval;
	    sigma=pow(10.0, sigma)*sqrt(double(Nbins)/Tobs)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

	    //printf("CheckBat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1],((MNStruct *)globalcontext)->pulse->obsn[nTOA].bat);
	    //printf("CheckSat %i  %.20Lg %.20Lg %.20Lg  \n", nTOA,((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0],((MNStruct *)globalcontext)->pulse->obsn[nTOA].sat );
	    long double binpos = ModelBats[nTOA];
	    if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;
	    long double minpos = binpos - FoldingPeriodDays/2;
	    //if(t==120){printf("minpos maxpos %i %.25Lg %.25Lg %.25Lg %.25Lg %.25Lg\n", t, ProfileBats[nTOA][0], ProfileBats[nTOA][Nbins-1], binpos, minpos, binpos + FoldingPeriodDays/2);}
	    if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
	    long double maxpos = binpos + FoldingPeriodDays/2;
	    if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];

	    //printf("Obvious time check: %.25Lg %.25Lg %.25Lg %.25Lg \n", ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0], FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][0][0]+FoldingPeriodDays, ((MNStruct *)globalcontext)->ProfileData[nTOA][Nbins-1][0]); 

	    for(int j =0; j < Nbins; j++){
		    long double timediff = 0;
		    long double bintime = ProfileBats[t][j];
		    if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
		    }
		    else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
		    }
		    else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
		    }

		    timediff=timediff*SECDAY;
	//	    timediff = timediff - 1.6674E-7 + 6.8229E-8 + 7.8479E-8;
	//	    if(t==120){printf("Bins: %i %i %.20Lg %.20Lg %.20Lg %.20Lg %.20Lg  \n", nTOA, j, minpos, maxpos, binpos, bintime, timediff);}
			if(j == 0){
                    //            printf("Model Bat: %i %s %g %g \n", t, ((MNStruct *)globalcontext)->pulse->obsn[t].fname , (double)ProfileBats[t][j], (double)timediff);
                        }


		    Betatimes[j]=timediff/beta;
		    TNothpl(numcoeff,Betatimes[j],Hermitepoly[j]);
		    for(int k =0; k < numcoeff; k++){
			    double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI)*((MNStruct *)globalcontext)->Factorials[k]);
			
				//printf("%i %i  %i %g %g %g %.25Lg %g %g\n", t, j, k, Hermitepoly[j][k], Bconst, Betatimes[j], timediff, beta,Hermitepoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]));

			    Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
		    }
	    }


	    double *shapevec  = new double[Nbins];
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');


		sigma=1;
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);
	            //printf("%i %i %g  %g \n", t, j, datadiff, shapevec[j]);
		    Chisq += datadiff*datadiff/(sigma*sigma);
		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
	    }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////
	
		
		double **M = new double*[Nbins];
		double **NM = new double*[Nbins];

		int Msize = 1 + numcoeff;

		for(int i =0; i < Nbins; i++){
		    M[i] = new double[Msize];
		    NM[i] = new double[Msize];
		    
		    M[i][0] = 1;

		    for(int k =0; k < numcoeff; k++){
			
			M[i][1+k] = Hermitepoly[i][k];
		    }

		    for(int k =0; k < Msize; k++){
		   	 NM[i][k] = M[i][k]/(sigma*sigma);
		    }

		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');

		for(int k =0; k < numcoeff; k++){
			for(int i =0; i < Nbins; i++){
				//printf("H: %i %i %g \n", i, k, Hermitepoly[i][k]);
			}
		}
		for(int i =0; i < Msize; i++){
			for(int j =0; j < Msize; j++){
				//printf("M: %i %i %g \n", i, j, MNM[i][j]);
			}
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];


		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
			MNM[i][i] += pow(10.0, -12)*MNM[i][i];
		}

		


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


		

		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');
			    

	      
	      
		Chisq = 0;
		detN = 0;

		double profnoise = sqrt(double(Nbins)/Tobs)*EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];


		double MLSigma = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
			double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - shapevec[j];
			//printf("%i %g %g \n", j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] , shapevec[j]);
			MLSigma += res*res; MLSigmaCount += 1;
		}
		MLSigma=sqrt(MLSigma/MLSigmaCount)*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		//printf("MLSigma: %g \n", MLSigma);
		for(int j =0; j < Nbins; j++){

			double noise = MLSigma*MLSigma;

			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

			if(shapevec[j] < 0)badshape = 1;
			NDiffVec[j] = datadiff/(noise);
			Chisq += datadiff*datadiff/(noise);
			
		 	for(int k =0; k < Msize; k++){
				NM[j][k] = M[j][k]/(noise);
		    	}


		
		}
		
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
			MNM[i][i] += pow(10.0, -12)*MNM[i][i];
		}


		info=0;
		Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


	    
		logMargindet += Margindet;

		Marginlike = 0;
		for(int i =0; i < Msize; i++){
			Marginlike += TempdNM[i]*dNM[i];
		}
		//printf("%g %g %g %g \n", detN, Chisq, logMargindet, Marginlike);
		profilelike = -0.5*(detN + Chisq + logMargindet - Marginlike);
		lnew += profilelike;

		delete[] shapevec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] TempdNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	 }
	 
	 
	 
	 

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}
	if(isnan(lnew)){
		lnew= -pow(10.0, 10.0);
	}
	printf("Like: %.10g \n", lnew);
	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;


	return lnew;

}


double  AllTOAWriteMaxLike(std::string longname, int &ndim){



	double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
        std::string checkname = longname+"_phys_live.txt";
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

	std::ifstream summaryfile;
	std::string fname = longname+"_phys_live.txt";
	summaryfile.open(fname.c_str());



	printf("Getting ML \n");
	double maxlike = -1.0*pow(10.0,10);
	for(int i=0;i<number_of_lines;i++){

		std::string line;
		getline(summaryfile,line);
		std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);

		double like = paramlist[ndim];

		if(like > maxlike){
			maxlike = like;
			 for(int i = 0; i < ndim; i++){
                        	 Cube[i] = paramlist[i];
                	 }
		}

	}
	summaryfile.close();

	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
	//	printf("timing param %i %.25Lg	\n", p, LDparams[pcount]);
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;

	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       /* Form residuals */




/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Red Noise///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }

	double maxtspan=1*(end-start);


	
	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int totCoeff=0;
	if(((MNStruct *)globalcontext)->incRED != 0)totCoeff+=FitRedCoeff;



	double FreqDet = 0;

	double *freqs = new double[totCoeff];
   	double *SignalCoeff = new double[totCoeff];

	for(int i = 0; i < totCoeff; i++){
		SignalCoeff[i] = 0;
	}	


	double *powercoeff=new double[totCoeff];
	for(int o=0;o<totCoeff; o++){
		powercoeff[o]=0;
	}
	
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			FMatrix[i][j]=0;
		}
	}


	int startpos=0;
	if(((MNStruct *)globalcontext)->incRED==5){

		FreqDet=0;

		for(int i = 0; i < FitRedCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			pcount++;
		}
			
		
		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitRedPL; pl ++){

			double Tspan = maxtspan;
                        double f1yr = 1.0/3.16e7;


			double redamp=Cube[pcount];
			pcount++;
			double redindex=Cube[pcount];
			pcount++;

			
			redamp=pow(10.0, redamp);
			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm +=log(redamp); }



			double Agw=redamp;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
				freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];
				
				double rho=0;

 				rho = (Agw*Agw/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-redindex))/(maxtspan*24*60*60);

 				powercoeff[i]+= rho;
 				powercoeff[i+FitRedCoeff/2]+= rho;

				

			}
		}
		
		
		double GWBAmp=0;
		if(((MNStruct *)globalcontext)->incGWB==1){
			GWBAmp=pow(10.0,Cube[pcount]);
			pcount++;

			uniformpriorterm += log(GWBAmp);
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				double rho = (GWBAmp*GWBAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-4.333))/(maxtspan*24*60*60);
				powercoeff[i]+= rho;
				powercoeff[i+FitRedCoeff/2]+= rho;
			}
		}

		for (int i=0; i<FitRedCoeff/2; i++){
			FreqDet=FreqDet+2*log(powercoeff[i]);
		}
		
		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);

			}
		}

		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
			}
		}

		startpos=FitRedCoeff;

    	}

	double *SignalVec = new double[((MNStruct *)globalcontext)->pulse->nobs];

	
	dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->pulse->nobs,totCoeff,'N');

	double FreqLike = 0;
	for(int i = 0; i < totCoeff; i++){
		FreqLike += SignalCoeff[i]*SignalCoeff[i]/powercoeff[i];
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr  - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;

	double shapecoeff[numcoeff];
	double StocProfCoeffs[numshapestoccoeff];

	for(int i =0; i < numcoeff; i++){
		if(((MNStruct *)globalcontext)->FixProfile==0){
			shapecoeff[i]= Cube[pcount];
			pcount++;
		}
		else{
			shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
			printf("Fixing mean: %i %g n", i, shapecoeff[i]);
		}
	}
	
	double beta=0;
	if(((MNStruct *)globalcontext)->FixProfile==0){
		beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;
		pcount++;
	}
	else{
		beta = ((MNStruct *)globalcontext)->MeanProfileBeta*((MNStruct *)globalcontext)->ReferencePeriod;
	}	

	for(int i =0; i < numshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		//uniformpriorterm += log(pow(10.0, Cube[pcount]));
		pcount++;
	}

	if(numcoeff+1>=numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double lnew = 0;
	int badshape = 0;

	double *meanshape = new double[numcoeff];
	double *maxshape = new double[numcoeff];
	double *minshape = new double[numcoeff];

	 for(int c = 0; c < numcoeff; c++){
		meanshape[c] = 0;
		maxshape[c] = -1000;
		minshape[c] = 1000;
	}
			
        int minoffpulse=750;
        int maxoffpulse=1250;
       //int minoffpulse=500;
       //int maxoffpulse=1500;

	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){

		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 
		double JitterPrior = 0;	   

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **AllHermiteBasis =  new double*[Nbins];
		double **JitterBasis  =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
		for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){JitterBasis[i][j]=0;}}
	
	



	

		double sigma=noiseval;
		sigma=pow(10.0, sigma);
		//printf("noise details %i %g %g %g \n", t, noiseval, Tobs, EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]]);

		long double binpos = ModelBats[nTOA];

		if(((MNStruct *)globalcontext)->incRED==5){
			long double rednoiseshift = (long double) SignalVec[t];
			binpos+=rednoiseshift/SECDAY;
			//printf("Red noise shift %i %Lg \n", t, rednoiseshift);
		}

		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    
		for(int j =0; j < Nbins; j++){
			long double timediff = 0;
			long double bintime = ProfileBats[t][j];
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;


			Betatimes[j]=timediff/beta;
			TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

			for(int k =0; k < maxshapecoeff; k++){

				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));//*((MNStruct *)globalcontext)->Factorials[k]);
				AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);
			//	if(nTOA==0 && j == 0){printf("%i %g \n", k, AllHermiteBasis[j][k]);}
				if(k<numcoeff){ Hermitepoly[j][k] = AllHermiteBasis[j][k];}
			
			}

			JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1]);
			for(int k =1; k < numcoeff; k++){
				JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1] - sqrt(double(k+1))*AllHermiteBasis[j][k+1]);
			}	
	    }


	    double *shapevec  = new double[Nbins];
	    double *Jittervec = new double[Nbins];
	
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
    	    dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');
	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    double modelflux=0;
	    double testmodelflux=0;	
            double dataflux = 0;


	    for(int j =0; j < Nbins; j++){

		    detN += log(sigma*sigma);

		    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] ;//- OverallProfileAmp*shapevec[j];//-offset;

		    if(shapevec[j] < 0)badshape = 1;
		    NDiffVec[j] = datadiff/(sigma*sigma);

		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}


		    testmodelflux += shapevec[j]*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;

	    }
	    for(int j =0; j < numcoeff; j=j+2){
		modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[j];
		//printf("binomial %i %g %g\n", j, ((MNStruct *)globalcontext)->Binomial[j], ((MNStruct *)globalcontext)->Factorials[j]/(((MNStruct *)globalcontext)->Factorials[j - j/2]*((MNStruct *)globalcontext)->Factorials[j/2]));
	   }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];
		double **NM = new double*[Nbins];

		int Msize = 2+numshapestoccoeff;
		int Mcounter=0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			Msize++;
		}


		for(int i =0; i < Nbins; i++){
			Mcounter=0;
			M[i] = new double[Msize];
			NM[i] = new double[Msize];

			M[i][0] = 1;
			M[i][1] = shapevec[i];

			Mcounter = 2;

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
//				printf("%i %i %g %g %g \n", nTOA, i, M[i][0], M[i][1], M[i][2]);
				M[i][Mcounter] = Jittervec[i];
				Mcounter++;
			}			



			for(int j = 0; j < numshapestoccoeff; j++){

			    M[i][Mcounter+j] = AllHermiteBasis[i][j];

			}

			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(sigma*sigma);
		//		printf("M %i %i %g %g \n",i,j,M[i][j], sigma); 
			}
		  
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');
//		printf("MNM %g %g %g %g \n", MNM[0][0], MNM[0][1], MNM[1][0], MNM[1][1]);

		for(int j = 2; j < Msize; j++){
			MNM[j][j] += pow(10.0,20);
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


		double maxoffset = TempdNM[0];
		double maxamp = TempdNM[1];



		printf("max %i %g %g \n", t, maxoffset, maxamp);

		      
		Chisq = 0;
		detN = 0;

		double noisemean=0.0;

		for(int j =minoffpulse; j < maxoffpulse; j++){
			noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
		}

		noisemean=noisemean/(maxoffpulse-minoffpulse);
		double MLSigma = 0;
		double MLSigma2 = 0;
		int MLSigmaCount2 = 0;
		int MLSigmaCount = 0;
		for(int j =0; j < Nbins; j++){
		      double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxamp*shapevec[j] - maxoffset;
		      //if(shapevec[j] <= maxshape*0.001){MLSigma += res*res; MLSigmaCount += 1; printf("Adding to Noise %i %i %g %g \n", t, j,(double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1], maxamp*shapevec[j] + maxoffset);}
			if(j > minoffpulse && j < maxoffpulse){
			//	if(nTOA == 0){printf("res: %i %g \n", j, res);}
				MLSigma += ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)*((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean); MLSigmaCount += 1;
			}
			else{
				MLSigma2 += res*res; MLSigmaCount2 += 1;
			}
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);
		MLSigma2 = sqrt(MLSigma2/MLSigmaCount2);
		double tempdataflux=0;
		for(int j =0; j < Nbins; j++){
			if(shapevec[j] > maxshape*0.001){
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - maxoffset)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
					tempdataflux += ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-maxoffset);
			//		if(t==1){
                          //                      printf("dataflux %i %g %g %g %g %g\n", j, maxoffset, ((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]), dataflux, tempdataflux, double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24);
                            //            }

				}
			}
		}	

 
//		printf("TOA %i %g %g %g %g\n", t, EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]], MLSigma, MLSigma2, MLSigma2/MLSigma);
		MLSigma = MLSigma;//*EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

//		double highfreqnoise = sqrt(double(Nbins)/Tobs)*SQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double highfreqnoise = SQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double flatnoise = EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];

//		double *testflux=new double[numshapestoccoeff];
//		for(int j = 0; j < numshapestoccoeff; j++){
//			testflux[j] = 0;
//		}



		double *profilenoise = new double[Nbins];
		for(int i =0; i < Nbins; i++){
		  	Mcounter=2;
			double noise = MLSigma*MLSigma +  pow(highfreqnoise*maxamp*shapevec[i],2);

			if(shapevec[i] > maxshape*0.001){
				noise +=  pow(flatnoise*maxamp*maxshape,2);
			}


			profilenoise[i] = sqrt(noise);
			//printf("detN: %i %i %g %g %g %g\n", nTOA, i, MLSigma, highfreqnoise, maxamp, noise);
			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];

			if(shapevec[i] < 0)badshape = 1;
			NDiffVec[i] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
	
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				M[i][2] = M[i][2]*dataflux/modelflux/beta;
				Mcounter++;
			}
			
		   	for(int j = 0; j < numshapestoccoeff; j++){
//				testflux[j] += pow(M[i][Mcounter+j]*sqrt(double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24)*dataflux, 2);
//				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux*sqrt(Tobs/double(Nbins));
				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux;

		    	}


			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(noise);
			}


		}
		
//		for(int j = 0; j < numshapestoccoeff; j++){
//			printf("test flux %i %i %g %g \n", t, j, testflux[j], pow(dataflux,2));
//		}
//		delete[] testflux;
			
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


		Mcounter=2;
		double JitterDet = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];//sqrt(double(Nbins)/Tobs)*EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			profnoise = profnoise*profnoise;
		    	JitterDet = log(profnoise);
		    	MNM[2][2] += 1.0/profnoise;
			Mcounter++;
		}


		double StocProfDet = 0;
		for(int j = 0; j < numshapestoccoeff; j++){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[Mcounter+j][Mcounter+j] += 1.0/StocProfCoeffs[j];
		}

		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		info=0;
		Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);

		Marginlike = 0;
                for(int i =0; i < Msize; i++){
                        Marginlike += TempdNM[i]*dNM[i];
                }



		double finaloffset = TempdNM[0];
		double finalamp = TempdNM[1];
		double finalJitter = TempdNM[2];

		double *StocVec = new double[Nbins];


		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');


		 for(int c = 0; c < numcoeff; c++){
			meanshape[c] += shapecoeff[c];
		}
		

		TempdNM[0] = 0;
		TempdNM[1] = 0;
		TempdNM[2] = 0;

                 for(int c = 3; c < Msize; c++){
                        meanshape[c-3] += (TempdNM[c]/finalamp)*dataflux;
			//printf("stoc: %i %g %g\n", c, (TempdNM[c]/finalamp)*dataflux, (TempdNM[c]/finalamp)/dataflux);
                }


		dgemv(M,TempdNM,StocVec,Nbins,Msize,'N');

		std::ofstream profilefile;
		std::string ProfileName =  ((MNStruct *)globalcontext)->pulse->obsn[t].fname;
		std::string dname = longname+ProfileName+"-Profile.txt";
	
		profilefile.open(dname.c_str());
		double MLSigma3 = 0;	
		double MLWeight3 = 0;	
		for(int i =0; i < Nbins; i++){
			MLSigma3 += pow(((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1]-shapevec[i]),2)*pow(1.0/profilenoise[i], 2);
			MLWeight3 += pow(1.0/profilenoise[i], 2);
			profilefile << i << " " << std::setprecision(10) << (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] << " " << shapevec[i] << " " << profilenoise[i] << " " << finaloffset + finalamp*M[i][1] << " " << finalJitter*M[i][2] << " " << finaloffset + StocVec[i] << "\n";

		}
		printf("MLsigma: %s %g %g %g \n", ProfileName.c_str(), MLSigma, MLSigma2, MLSigma2/MLSigma);
	    	profilefile.close();
		delete[] profilenoise;
		delete[] StocVec;

		logMargindet = Margindet;


		profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
		lnew += profilelike;
//		printf("Like: %i  %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

		delete[] shapevec;
		delete[] Jittervec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] AllHermiteBasis[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] AllHermiteBasis;
		delete[] JitterBasis;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 for(int c = 0; c < numcoeff; c++){
		double mval = meanshape[c]/meanshape[0];
		printf("ShapePriors[%i][0] = %.10g         %.10g\n", c, mval-mval*0.5, mval);
		printf("ShapePriors[%i][1] = %.10g         %.10g\n",c, mval+mval*0.5, mval);
	}
	 

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] powercoeff;
	delete[] freqs;
	delete[] SignalCoeff;
	delete[] SignalVec;
	
	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[]FMatrix[j];
	}
	delete[]FMatrix;


	printf("lnew: %g \n", lnew);

	return lnew;

}


double  AllTOAMarginStocProfLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	printf("in like\n");

	double *EFAC;
	double *EQUAD;
        long double LDparams[ndim];
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int fitcount=0;
	for(int p=0;p< ((MNStruct *)globalcontext)->numFitTiming + ((MNStruct *)globalcontext)->numFitJumps; p++){

		LDparams[p]=Cube[fitcount]*(((MNStruct *)globalcontext)->LDpriors[p][1]) + (((MNStruct *)globalcontext)->LDpriors[p][0]);
		fitcount++;

	}	
	
	
	pcount=0;
	long double phase = LDparams[0]*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	for(int p=1;p<((MNStruct *)globalcontext)->numFitTiming;p++){
		((MNStruct *)globalcontext)->pulse->param[((MNStruct *)globalcontext)->TempoFitNums[p][0]].val[((MNStruct *)globalcontext)->TempoFitNums[p][1]] = LDparams[pcount];
	//	printf("timing param %i %.25Lg	\n", p, LDparams[pcount]);
		pcount++;
	}
	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= LDparams[pcount];
		pcount++;
	}

	long double *JumpVec = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){

	   JumpVec[p] = 0;
           for (int k=1;k<=((MNStruct *)globalcontext)->pulse->nJumps;k++){
           	for (int l=0;l<((MNStruct *)globalcontext)->pulse->obsn[p].obsNjump;l++){
                	if(((MNStruct *)globalcontext)->pulse->obsn[p].jump[l]==k){
                        	JumpVec[p] += (long double) ((MNStruct *)globalcontext)->pulse->jumpVal[k]/SECDAY;
			}
                 }
           }

	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase + JumpVec[p];
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }

	delete[] JumpVec;

	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       /* Form residuals */




/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  





/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Red Noise///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double start,end;
	int go=0;
	for (int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++)
	  {
	    if (((MNStruct *)globalcontext)->pulse->obsn[i].deleted==0)
	      {
		if (go==0)
		  {
		    go = 1;
		    start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    end  = start;
		  }
		else
		  {
		    if (start > (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      start = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		    if (end < (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat)
		      end = (double)((MNStruct *)globalcontext)->pulse->obsn[i].bat;
		  }
	      }
	  }

	double maxtspan=1*(end-start);


	
	int FitRedCoeff=2*(((MNStruct *)globalcontext)->numFitRedCoeff);
	int totCoeff=0;
	if(((MNStruct *)globalcontext)->incRED != 0)totCoeff+=FitRedCoeff;



	double FreqDet = 0;

	double *freqs = new double[totCoeff];
   	double *SignalCoeff = new double[totCoeff];

	for(int i = 0; i < totCoeff; i++){
		SignalCoeff[i] = 0;
	}	


	double *powercoeff=new double[totCoeff];
	for(int o=0;o<totCoeff; o++){
		powercoeff[o]=0;
	}
	
	double **FMatrix=new double*[((MNStruct *)globalcontext)->pulse->nobs];
	for(int i=0;i<((MNStruct *)globalcontext)->pulse->nobs;i++){
		FMatrix[i]=new double[totCoeff];
		for(int j=0;j<totCoeff;j++){
			FMatrix[i][j]=0;
		}
	}


	int startpos=0;
	if(((MNStruct *)globalcontext)->incRED==5){

		FreqDet=0;

		for(int i = 0; i < FitRedCoeff; i++){
			SignalCoeff[startpos + i] = Cube[pcount];
			pcount++;
		}
			
		
		for(int pl = 0; pl < ((MNStruct *)globalcontext)->numFitRedPL; pl ++){

			double Tspan = maxtspan;
                        double f1yr = 1.0/3.16e7;


			double redamp=Cube[pcount];
			pcount++;
			double redindex=Cube[pcount];
			pcount++;

			
			redamp=pow(10.0, redamp);
			if(((MNStruct *)globalcontext)->RedPriorType ==1) { uniformpriorterm +=log(redamp); }



			double Agw=redamp;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				
				freqs[startpos+i]=(double)((MNStruct *)globalcontext)->sampleFreq[i]/maxtspan;
				freqs[startpos+i+FitRedCoeff/2]=freqs[startpos+i];
				
				double rho=0;

 				rho = (Agw*Agw/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-redindex))/(maxtspan*24*60*60);

 				powercoeff[i]+= rho;
 				powercoeff[i+FitRedCoeff/2]+= rho;

				

			}
		}
		
		
		double GWBAmp=0;
		if(((MNStruct *)globalcontext)->incGWB==1){
			GWBAmp=pow(10.0,Cube[pcount]);
			pcount++;

			uniformpriorterm += log(GWBAmp);
			double Tspan = maxtspan;
			double f1yr = 1.0/3.16e7;
			for (int i=0; i<FitRedCoeff/2 - ((MNStruct *)globalcontext)->incFloatRed ; i++){
				double rho = (GWBAmp*GWBAmp/12.0/(M_PI*M_PI))*pow(f1yr,(-3)) * pow(freqs[i]*365.25,(-4.333))/(maxtspan*24*60*60);
				powercoeff[i]+= rho;
				powercoeff[i+FitRedCoeff/2]+= rho;
			}
		}

		for (int i=0; i<FitRedCoeff/2; i++){
			FreqDet=FreqDet+2*log(powercoeff[i]);
		}
		
		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i]=cos(2*M_PI*freqs[i]*time);

			}
		}

		for(int i=0;i<FitRedCoeff/2;i++){
			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;
				FMatrix[k][i+FitRedCoeff/2]=sin(2*M_PI*freqs[i]*time);
			}
		}

		startpos=FitRedCoeff;

    	}

	double *SignalVec = new double[((MNStruct *)globalcontext)->pulse->nobs];
	for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){
		SignalVec[k] = 0;
	}

	
	dgemv(FMatrix,SignalCoeff,SignalVec,((MNStruct *)globalcontext)->pulse->nobs,totCoeff,'N');

	double FreqLike = 0;
	for(int i = 0; i < totCoeff; i++){
		FreqLike += SignalCoeff[i]*SignalCoeff[i]/powercoeff[i];
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Subtract  DM Shape Events////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////  

	if(((MNStruct *)globalcontext)->incDMShapeEvent != 0){
                for(int i =0; i < ((MNStruct *)globalcontext)->incDMShapeEvent; i++){

			int numDMShapeCoeff=((MNStruct *)globalcontext)->numDMShapeCoeff;

                        double EventPos=Cube[pcount];
			pcount++;
                        double EventWidth=Cube[pcount];
			pcount++;



			double *DMshapecoeff=new double[numDMShapeCoeff];
			double *DMshapeVec=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapecoeff[c]=Cube[pcount];
				pcount++;
			}


			double *DMshapeNorm=new double[numDMShapeCoeff];
			for(int c=0; c < numDMShapeCoeff; c++){
				DMshapeNorm[c]=1.0/sqrt(sqrt(2.0*M_PI)*pow(2.0,c)*iter_factorial(c));
			}

			for(int k=0;k<((MNStruct *)globalcontext)->pulse->nobs;k++){	
				double time=(double)((MNStruct *)globalcontext)->pulse->obsn[k].bat;

				double HVal=(time-EventPos)/(sqrt(2.0)*EventWidth);
				TNothpl(numDMShapeCoeff,HVal,DMshapeVec);
				double DMsignal=0;
				for(int c=0; c < numDMShapeCoeff; c++){
					DMsignal += DMshapeNorm[c]*DMshapeVec[c]*DMshapecoeff[c];
				}

				  SignalVec[k] += DMsignal*exp(-0.5*pow((time-EventPos)/EventWidth, 2));
	
			}




		delete[] DMshapecoeff;
		delete[] DMshapeVec;
		delete[] DMshapeNorm;

		}
	}



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr;
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr  - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;


	double StocProfCoeffs[numshapestoccoeff];
	double profileamps[((MNStruct *)globalcontext)->pulse->nobs];

	profileamps[0] = 1;
	for(int i =1; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
		profileamps[i]= Cube[pcount];
		pcount++;
	}

	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;

	pcount++;

	for(int i =0; i < numshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		//uniformpriorterm += log(pow(10.0, Cube[pcount]));
		pcount++;
	}

	if(numcoeff+1>numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	printf("starting profile stuff \n");
	double lnew = 0;
	int badshape = 0;

	int totalcoeff = numcoeff;
	int totalbins = 0;
	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
		totalbins  += (int)((MNStruct *)globalcontext)->ProfileInfo[t][1];
		totalcoeff += numshapestoccoeff;
		totalcoeff += 1;    //For the arbitrary offset
	}

	double *AllNoise = new double[totalbins];
	double *AllD = new double[totalbins];
	double *dNM = new double[totalcoeff];
	double *TempdNM = new double[totalcoeff];

	double **AllProfiles = new double*[totalcoeff];
	double **NAllProfiles = new double*[totalcoeff];
	for(int t = 0; t < totalcoeff; t++){
		AllProfiles[t] = new double[totalbins];
		NAllProfiles[t] = new double[totalbins];
		for(int i = 0; i < totalcoeff; i++){
			AllProfiles[t][i] = 0;
			NAllProfiles[t][i] = 0;
		}
	}
	
	int stocprofcount = numcoeff+((MNStruct *)globalcontext)->pulse->nobs;
	int bincount = 0;
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){



		double MLSigma = 0;
		int MLSigmaCount = 0;
		double noisemean=0;

		for(int j =500; j < 1500; j++){
			noisemean += (double)((MNStruct *)globalcontext)->ProfileData[t][j][1];
			MLSigmaCount += 1;
		}

		noisemean = noisemean/MLSigmaCount;

		for(int j =500; j < 1500; j++){
			double res = (double)((MNStruct *)globalcontext)->ProfileData[t][j][1] - noisemean;
			MLSigma += res*res; 
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);


		printf("toa %i sig %g \n", t, MLSigma);

		int nTOA = t;


		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **AllHermiteBasis =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}

	

		long double binpos = ModelBats[nTOA];

	//	if(((MNStruct *)globalcontext)->incRED==5){
			long double rednoiseshift = (long double) SignalVec[t];
			binpos+=rednoiseshift/SECDAY;
			//printf("Red noise shift %i %Lg \n", t, rednoiseshift);
	//	}

		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    	double flatnoise = EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]]*profileamps[t];


		for(int j =0; j < Nbins; j++){

			AllD[bincount+j] =  (double)((MNStruct *)globalcontext)->ProfileData[t][j][1];

			long double timediff = 0;
			long double bintime = ProfileBats[t][j];
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;


			Betatimes[j]=timediff/beta;
			TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

			//printf("%i %i %g \n", t, j, AllD[bincount+j]);
			for(int k =0; k < maxshapecoeff; k++){
				//printf("%i %i %i %i %i\n", k, stocprofcount+k, bincount+j, totalcoeff, totalbins);
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));
				AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

				if(k < numcoeff){ 
					AllProfiles[k][bincount+j] = AllHermiteBasis[j][k]*profileamps[t];
				}

				if(k < numshapestoccoeff){ 
					AllProfiles[stocprofcount+k][bincount+j] = AllHermiteBasis[j][k]*profileamps[t];
				}
			
			}

			AllProfiles[numcoeff+t][bincount+j] = 1;


			AllNoise[bincount+j] = MLSigma*MLSigma;
			if(j < 500 || j > 1500){ 
				AllNoise[bincount+j] += flatnoise*flatnoise;
			}



	   	 }

		stocprofcount += numshapestoccoeff;
		bincount += Nbins;

		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] AllHermiteBasis[j];
		}
		delete[] Hermitepoly;
		delete[] AllHermiteBasis;

	
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Do the Linear Algebra///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	int SVDSize = numcoeff+((MNStruct *)globalcontext)->pulse->nobs;
	double **M = new double*[totalbins];
	for(int j =0; j < totalbins; j++){
	 	M[j] = new double[SVDSize];
		for(int k =0; k < SVDSize; k++){
			M[j][k] = AllProfiles[k][j];
		}
	}
	 

	double* S = new double[SVDSize];
	double** U = new double*[totalbins];
	for(int k=0; k < totalbins; k++){
		U[k] = new double[totalbins];
	}
	double** VT = new double*[SVDSize]; 
	for (int k=0; k<SVDSize; k++) VT[k] = new double[SVDSize];

	printf("doing the svd \n"); 
	dgesvd(M,totalbins, SVDSize, S, U, VT);
	printf("done\n");

	for(int j =0; j < totalbins; j++){
		for(int k =0; k < SVDSize; k++){
			AllProfiles[k][j] = U[j][k];
		}
	}

	delete[]S;	

	for (int j = 0; j < SVDSize; j++){
		delete[]VT[j];
	}

	delete[]VT;	


 
	for (int j = 0; j < totalbins; j++){
		delete[] U[j];
		delete[] M[j];
	}

	delete[]U;
	delete[]M;


	for(int j =0; j < totalbins; j++){
		for(int k =0; k < SVDSize; k++){
			AllProfiles[k][j] = U[j][k];
		}
	}


	double detN = 0;
	for(int j =0; j < totalbins; j++){
		detN += log(AllNoise[j]);
		for(int k =0; k < totalcoeff; k++){
			NAllProfiles[k][j] = AllProfiles[k][j]/AllNoise[j];
		}
	}

	 dgemv(NAllProfiles,AllD,dNM,totalcoeff,totalbins,'N');

	double **MNM = new double*[totalcoeff];
	for(int i =0; i < totalcoeff; i++){
		MNM[i] = new double[totalcoeff];
	}

	printf("doing dgemm \n");
	dgemm(NAllProfiles, AllProfiles , MNM, totalcoeff, totalbins,totalcoeff, totalbins, 'N', 'T');
 	printf("done\n");

	stocprofcount = numcoeff+((MNStruct *)globalcontext)->pulse->nobs;
	double StocProfDet = 0;
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
		for(int j = 0; j < numshapestoccoeff; j++){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[stocprofcount+j][stocprofcount+j] += 1.0/StocProfCoeffs[j];
		}

		stocprofcount += numshapestoccoeff;
	}

	int info=0;
	double Margindet = 0;
	dpotrfInfo(MNM, totalcoeff, Margindet, info);
	dpotrs(MNM, TempdNM, totalcoeff);

	double Marginlike = 0;
        for(int i =0; i < totalcoeff; i++){
                Marginlike += TempdNM[i]*dNM[i];
        }

	lnew = -0.5*(detN + StocProfDet + Margindet - Marginlike);

	if(badshape == 1){
	//	lnew = -pow(10.0, 10.0);
	}

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;
	delete[] powercoeff;
	delete[] freqs;
	delete[] SignalCoeff;
	delete[] SignalVec;
	
	for (int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
		delete[]FMatrix[j];
	}
	delete[]FMatrix;


	lnew += -0.5*(FreqDet + FreqLike) + uniformpriorterm;
	printf("Like: %.10g %g %g %g %g\n", lnew, detN , StocProfDet , Margindet , Marginlike);

	return lnew;

}



void TemplateProfLikeMNWrap(double *Cube, int &ndim, int &npars, double &lnew, void *context){



        for(int p=0;p<ndim;p++){

                Cube[p]=(((MNStruct *)globalcontext)->PriorsArray[p+ndim]-((MNStruct *)globalcontext)->PriorsArray[p])*Cube[p]+((MNStruct *)globalcontext)->PriorsArray[p];
        }


	double *DerivedParams = new double[npars];

	double result = TemplateProfLike(ndim, Cube, npars, DerivedParams, context);


	delete[] DerivedParams;

	lnew = result;

}

double  TemplateProfLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){

	//printf("In like \n");

	double *EFAC;
	double *EQUAD;
	double TemplateFlux=0;
	double FakeRMS = 0;
	double TargetSN=0;
        long double LDparams;
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	LDparams=Cube[0]*(((MNStruct *)globalcontext)->LDpriors[0][1]) + (((MNStruct *)globalcontext)->LDpriors[0][0]);
	
	
	pcount=0;
	long double phase = LDparams*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	TemplateFlux=0;
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0];
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileData[i][0][0]+phase;
	 }

	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;

	pcount++;

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double lnew = 0;
	int badshape = 0;

	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	//	printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
	

		long double binpos = ModelBats[nTOA];


		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    
		for(int j =0; j < Nbins; j++){
			long double timediff = 0;
			long double bintime = ProfileBats[t][j];
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;

			Betatimes[j]=timediff/beta;
			TNothpl(numcoeff,Betatimes[j],Hermitepoly[j]);

			for(int k =0; k < numcoeff; k++){
				//if(k==0)printf("HP %i %g \n", j, Betatimes[j]);
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));//*((MNStruct *)globalcontext)->Factorials[k]);
				Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

			}
	    }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];

		int Msize = numcoeff+1;
		for(int i =0; i < Nbins; i++){
			M[i] = new double[Msize];

			M[i][0] = 1;


			for(int j = 0; j < numcoeff; j++){
				M[i][j+1] = Hermitepoly[i][j];
				//if(j==0)printf("%i %i %g \n", i, j, M[i][j+1]);
			}
		  
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		dgemm(M, M , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');

		for(int j = 0; j < Msize; j++){
			MNM[j][j] += pow(10.0,-14);
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];

		double *NDiffVec = new double[Nbins];
		TemplateFlux = 0;
		for(int j = 0; j < Nbins; j++){
			TemplateFlux +=  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
			NDiffVec[j] = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
		}
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


//		printf("Margindet %g \n", Margindet);
		double *shapevec = new double[Nbins];
		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');


		TargetSN = 10000.0;
		FakeRMS = TemplateFlux/TargetSN;
		FakeRMS = 1.0/(FakeRMS*FakeRMS);
		    double Chisq=0;
		    for(int j =0; j < Nbins; j++){


			    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - shapevec[j];
			    Chisq += datadiff*datadiff*FakeRMS;
//				printf("%i %.10g %.10g \n", j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] , shapevec[j]);		

		    }

		profilelike = -0.5*(Chisq);
		lnew += profilelike;

		delete[] shapevec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] M[j];
		}
		delete[] Hermitepoly;
		delete[] M;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;


	//printf("End Like: %.10g %g %g\n", lnew, TemplateFlux, FakeRMS);

	return lnew;

}

void  WriteMaxTemplateProf(std::string longname, int &ndim){

	//printf("In like \n");


        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+"phys_live.points";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+"_phys_live.txt";
        }
	printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+"phys_live.points";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+"_phys_live.txt";
	}
        summaryfile.open(fname.c_str());



        printf("Getting ML \n");
        double maxlike = -1.0*pow(10.0,10);
        for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);

                double like = paramlist[ndim];

                if(like > maxlike){
                        maxlike = like;
                         for(int i = 0; i < ndim; i++){
                                 Cube[i] = paramlist[i];
                         }
                }

        }
        summaryfile.close();


	double *EFAC;
	double *EQUAD;
	double TemplateFlux=0;
	double FakeRMS = 0;
	double TargetSN=0;
        long double LDparams;
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	LDparams=Cube[0]*(((MNStruct *)globalcontext)->LDpriors[0][1]) + (((MNStruct *)globalcontext)->LDpriors[0][0]);
	
	
	pcount=0;
	long double phase = LDparams*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	TemplateFlux=0;
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	   
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0];
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileData[i][0][0]+phase;
	 }

	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	double beta = Cube[pcount]*((MNStruct *)globalcontext)->ReferencePeriod;

	pcount++;

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	double lnew = 0;
	int badshape = 0;

	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
	//	printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
	

		long double binpos = ModelBats[nTOA];


		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    
		for(int j =0; j < Nbins; j++){
			long double timediff = 0;
			long double bintime = ProfileBats[t][j];
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;

			Betatimes[j]=timediff/beta;
			TNothpl(numcoeff,Betatimes[j],Hermitepoly[j]);

			for(int k =0; k < numcoeff; k++){
				//if(k==0)printf("HP %i %g \n", j, Betatimes[j]);
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));//*((MNStruct *)globalcontext)->Factorials[k]);
				Hermitepoly[j][k]=Hermitepoly[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

			}
	    }

	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];

		int Msize = numcoeff+1;
		for(int i =0; i < Nbins; i++){
			M[i] = new double[Msize];

			M[i][0] = 1;


			for(int j = 0; j < numcoeff; j++){
				M[i][j+1] = Hermitepoly[i][j];
				//if(j==0)printf("%i %i %g \n", i, j, M[i][j+1]);
			}
		  
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		dgemm(M, M , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');

		for(int j = 0; j < Msize; j++){
			MNM[j][j] += pow(10.0,-14);
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];

		double *NDiffVec = new double[Nbins];
		TemplateFlux = 0;
		for(int j = 0; j < Nbins; j++){
			TemplateFlux +=  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
			NDiffVec[j] = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
		}
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);


//		printf("Margindet %g \n", Margindet);
		double *shapevec = new double[Nbins];
		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');


		TargetSN = 10000.0;
		FakeRMS = TemplateFlux/TargetSN;
		FakeRMS = 1.0/(FakeRMS*FakeRMS);
		    double Chisq=0;
		    for(int j =0; j < Nbins; j++){


			    double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - shapevec[j];
			    Chisq += datadiff*datadiff*FakeRMS;
				printf("%i %.10g %.10g %.10g \n", j, (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] , shapevec[j], 1.0/sqrt(FakeRMS));		

		    }

		for(int j = 0; j < numcoeff; j++){
			printf("ShapePriors[%i][0] = %.10g \n", j, TempdNM[j+1]/TempdNM[1]);
			printf("ShapePriors[%i][1] = %.10g \n", j, TempdNM[j+1]/TempdNM[1]); 
		}
                printf("ShapePriors[%i][0] = %.10g \n", numcoeff, Cube[1]);
                printf("ShapePriors[%i][1] = %.10g \n", numcoeff, Cube[1]);


		profilelike = -0.5*(Chisq);
		lnew += profilelike;

		delete[] shapevec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] M[j];
		}
		delete[] Hermitepoly;
		delete[] M;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;


	//printf("End Like: %.10g %g %g\n", lnew, TemplateFlux, FakeRMS);


}


void SubIntStocProfLikeMNWrap(double *Cube, int &ndim, int &npars, double &lnew, void *context){



        for(int p=0;p<ndim;p++){

                Cube[p]=(((MNStruct *)globalcontext)->PriorsArray[p+ndim]-((MNStruct *)globalcontext)->PriorsArray[p])*Cube[p]+((MNStruct *)globalcontext)->PriorsArray[p];
        }


	double *DerivedParams = new double[npars];

	double result = SubIntStocProfLike(ndim, Cube, npars, DerivedParams, context);


	delete[] DerivedParams;

	lnew = result;

}

double  SubIntStocProfLike(int &ndim, double *Cube, int &npars, double *DerivedParams, void *context){


	struct timeval tval_before, tval_after, tval_resultone, tval_resulttwo;


//	gettimeofday(&tval_before, NULL);


	int debug = 0;

	if(debug == 1)printf("In like \n");

	double *EFAC;
	double *EQUAD;
        long double LDparams;
        int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	LDparams=Cube[0]*(((MNStruct *)globalcontext)->LDpriors[0][1]) + (((MNStruct *)globalcontext)->LDpriors[0][0]);
	
	pcount=0;
	long double phase = LDparams*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	
	if(debug == 1)printf("Phase: %g \n", (double)phase);


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	//for(int i = 0; i < 10; i ++){
	//	double pval = i*0.1;
	//	phase = pval*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;

	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase;
	}


        for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
                ((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
        }


	fastformSubIntBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       /* Form residuals */

	//for(int j = 0; j < ((MNStruct *)globalcontext)->pulse->nobs; j++){
	//	printf("%i %g %i %.15g %.15g \n", i, pval, j, (double)((MNStruct *)globalcontext)->pulse->obsn[j].batCorr, (double)((MNStruct *)globalcontext)->pulse->obsn[j].residual	);
	//}/


	//}
	//sleep(5);

	if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
                EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
                        EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
                }
        }


        double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
                for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
        	SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
        	for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
           		SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
        	}
    	}	  
	  



/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	      //printf("badcorr? %i %g %g \n", i,  (double)((MNStruct *)globalcontext)->pulse->obsn[i].batCorr, (double)((MNStruct *)globalcontext)->pulse->obsn[i].residual);
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[0].batCorr;
		
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[0].batCorr - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;

	double shapecoeff[numcoeff];
	double StocProfCoeffs[numshapestoccoeff];

	for(int i =0; i < numcoeff; i++){
		shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
	}
	double beta = ((MNStruct *)globalcontext)->MeanProfileBeta*((MNStruct *)globalcontext)->ReferencePeriod;


	for(int i =0; i < numshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		pcount++;
	}

	if(numcoeff+1>=numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double modelflux=0;
	    for(int j =0; j < numcoeff; j=j+2){
		modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[j];
	   }

	double lnew = 0;
	int badshape = 0;

	int minoffpulse=750;
	int maxoffpulse=1250;
//        int minoffpulse=200;
//        int maxoffpulse=800;



	int GlobalNBins = (int)((MNStruct *)globalcontext)->ProfileInfo[0][1];


	double **AllHermiteBasis =  new double*[GlobalNBins];
	double **JitterBasis  =  new double*[GlobalNBins];
	double **Hermitepoly =  new double*[GlobalNBins];

	for(int i =0;i<GlobalNBins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];}
	for(int i =0;i<GlobalNBins;i++){Hermitepoly[i]=new double[numcoeff];}
	for(int i =0;i<GlobalNBins;i++){JitterBasis[i]=new double[numcoeff];}


	double **M = new double*[GlobalNBins];
	double **NM = new double*[GlobalNBins];

	int Msize = 2+numshapestoccoeff;
	int Mcounter=0;
	if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
		Msize++;
	}


	for(int i =0; i < GlobalNBins; i++){
		Mcounter=0;
		M[i] = new double[Msize];
		NM[i] = new double[Msize];

	}


	double **MNM = new double*[Msize];
	for(int i =0; i < Msize; i++){
	    MNM[i] = new double[Msize];
	}


//		gettimeofday(&tval_after, NULL);

//		timersub(&tval_after, &tval_before, &tval_resultone);


//		gettimeofday(&tval_before, NULL);
	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){

	
//		gettimeofday(&tval_before, NULL);

		if(debug == 1)printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 
		double JitterPrior = 0;	   

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
	

	        double *shapevec  = new double[Nbins];
	        double *Jittervec = new double[Nbins];

		double noisemean=0;
		for(int j =minoffpulse; j < maxoffpulse; j++){
			noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
		}

		noisemean = noisemean/(maxoffpulse-minoffpulse);

		double MLSigma = 0;
		int MLSigmaCount = 0;
		double dataflux = 0;
		for(int j = 0; j < Nbins; j++){
			if(j>=minoffpulse && j < maxoffpulse){
				double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean;
				MLSigma += res*res; MLSigmaCount += 1;
			}
			else{
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
				}
			}
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);
		double maxamp = dataflux/modelflux;

		if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, modelflux, maxamp);


		long double binpos = ModelBats[nTOA];

		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];



		int InterpBin = 0;
		double FirstInterpTimeBin = 0;
		int  NumWholeBinInterpOffset = 0;

		if(((MNStruct *)globalcontext)->InterpolateProfile == 1){

		
			long double timediff = 0;
			long double bintime = ProfileBats[t][0];


			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;

			double OneBin = FoldingPeriod/Nbins;
			int NumBinsInTimeDiff = floor(timediff/OneBin + 0.5);
			double WholeBinsInTimeDiff = NumBinsInTimeDiff*FoldingPeriod/Nbins;
			double OneBinTimeDiff = -1*((double)timediff - WholeBinsInTimeDiff);

			double PWrappedTimeDiff = (OneBinTimeDiff - floor(OneBinTimeDiff/OneBin)*OneBin);

			if(debug == 1)printf("Making InterpBin: %g %g %i %g %g %g\n", (double)timediff, OneBin, NumBinsInTimeDiff, WholeBinsInTimeDiff, OneBinTimeDiff, PWrappedTimeDiff);

			InterpBin = floor(PWrappedTimeDiff/((MNStruct *)globalcontext)->InterpolatedTime+0.5);
			if(InterpBin >= ((MNStruct *)globalcontext)->NumToInterpolate)InterpBin -= ((MNStruct *)globalcontext)->NumToInterpolate;

			FirstInterpTimeBin = -1*(InterpBin-1)*((MNStruct *)globalcontext)->InterpolatedTime;

			if(debug == 1)printf("Interp Time Diffs: %g %g %g %g \n", ((MNStruct *)globalcontext)->InterpolatedTime, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime, PWrappedTimeDiff, InterpBin*((MNStruct *)globalcontext)->InterpolatedTime-PWrappedTimeDiff);

			double FirstBinOffset = timediff-FirstInterpTimeBin;
			double dNumWholeBinOffset = FirstBinOffset/(FoldingPeriod/Nbins);
			int  NumWholeBinOffset = 0;

			NumWholeBinInterpOffset = floor(dNumWholeBinOffset+0.5);
	
			if(debug == 1)printf("Interp bin is: %i , First Bin is %g, Offset is %i \n", InterpBin, FirstInterpTimeBin, NumWholeBinInterpOffset);


		}
	   
		if(((MNStruct *)globalcontext)->InterpolateProfile == 0){

 
			for(int j =0; j < Nbins; j++){


				long double timediff = 0;
				long double bintime = ProfileBats[t][j];
					

				if(bintime  >= minpos && bintime <= maxpos){
				    timediff = bintime - binpos;
				}
				else if(bintime < minpos){
				    timediff = FoldingPeriodDays+bintime - binpos;
				}
				else if(bintime > maxpos){
				    timediff = bintime - FoldingPeriodDays - binpos;
				}


				timediff=timediff*SECDAY;


				Betatimes[j]=timediff/beta;
				TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

				for(int k =0; k < maxshapecoeff; k++){
					double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));
					AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

					if(k<numcoeff){ Hermitepoly[j][k] = AllHermiteBasis[j][k];}


			//		if(((MNStruct *)globalcontext)->InterpolateProfile == 1 && k == 0){	
			//			printf("Interped: %i %.10g %.10g %.10g \n", j, (double)timediff,AllHermiteBasis[j][k], ((MNStruct *)globalcontext)->InterpolatedShapelets[InterpBin][Nj][k]); 
			//		}
				}

				JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*Hermitepoly[j][1]);
				for(int k =1; k < numcoeff; k++){
					JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1] - sqrt(double(k+1))*AllHermiteBasis[j][k+1]);
				}

			}

			dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
			dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');

                        for(int j =0; j < Nbins; j++){

				
				M[j][0] = 1;
				M[j][1] = shapevec[j];

				Mcounter = 2;

				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					M[j][Mcounter] = Jittervec[j]*dataflux/modelflux/beta;
					Mcounter++;
				}

				for(int k = 0; k < numshapestoccoeff; k++){
				    M[j][Mcounter+k] = AllHermiteBasis[j][k]*dataflux;
				}

			}
		}

		if(((MNStruct *)globalcontext)->InterpolateProfile == 1){

			for(int j =0; j < Nbins; j++){


				double NewIndex = (j + NumWholeBinInterpOffset);
				int Nj = (int)(NewIndex - floor(NewIndex/Nbins)*Nbins);

				M[j][0] = 1;
				M[j][1] = ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj];

				Mcounter = 2;
				
				if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
					M[j][Mcounter] = ((MNStruct *)globalcontext)->InterpolatedJitterProfile[InterpBin][Nj]*dataflux/modelflux/beta;
					Mcounter++;
				}			

				for(int k = 0; k < numshapestoccoeff; k++){
				    M[j][Mcounter+k] = ((MNStruct *)globalcontext)->InterpolatedShapelets[InterpBin][Nj][k]*dataflux;
				}

				shapevec[j] = ((MNStruct *)globalcontext)->InterpolatedMeanProfile[InterpBin][Nj];


			}


	    }

	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;
	    for(int j =0; j < Nbins; j++){
 	        if(shapevec[j] > maxshape){ maxshape = shapevec[j];}
	    }


	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		      
		Chisq = 0;
		detN = 0;

	
 
		double highfreqnoise = SQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double flatnoise = EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];


		for(int i =0; i < Nbins; i++){
		  	Mcounter=2;
			double noise = MLSigma*MLSigma +  pow(highfreqnoise*maxamp*shapevec[i],2);

			if(shapevec[i] > maxshape*0.001){
				noise +=  pow(flatnoise*maxamp*maxshape,2);
			}
			detN += log(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];

			NDiffVec[i] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
	


			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(noise);
			}


		}
	
	
	//	gettimeofday(&tval_after, NULL);

	//	timersub(&tval_after, &tval_before, &tval_resultone);


	//	gettimeofday(&tval_before, NULL);
			
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


		Mcounter=2;
		double JitterDet = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			profnoise = profnoise*profnoise;
		    	JitterDet = log(profnoise);
		    	MNM[2][2] += 1.0/profnoise;
			Mcounter++;
		}


		double StocProfDet = 0;
		for(int j = 0; j < numshapestoccoeff; j++){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[Mcounter+j][Mcounter+j] +=  1.0/StocProfCoeffs[j];
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);
			    
		logMargindet = Margindet;

		Marginlike = 0;
		for(int i =0; i < Msize; i++){
			Marginlike += TempdNM[i]*dNM[i];
		}

		profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
		lnew += profilelike;
		if(debug == 1)printf("Like: %i %.15g %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

		delete[] shapevec;
		delete[] Jittervec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] TempdNM;
		delete[] Betatimes;


	
//        	gettimeofday(&tval_after, NULL);

  //      	timersub(&tval_after, &tval_before, &tval_resulttwo);
    //    	printf("Time elapsed: %ld.%06ld %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec, (long int)tval_resulttwo.tv_sec, (long int)tval_resulttwo.tv_usec);
	
	}
	 
	 
	for (int j = 0; j < GlobalNBins; j++){
	    delete[] Hermitepoly[j];
	    delete[] JitterBasis[j];
	    delete[] AllHermiteBasis[j];
	    delete[] M[j];
	    delete[] NM[j];
	}
	delete[] Hermitepoly;
	delete[] AllHermiteBasis;
	delete[] JitterBasis;
	delete[] M;
	delete[] NM;

	for (int j = 0; j < Msize; j++){
	    delete[] MNM[j];
	}
	delete[] MNM;
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;


	lnew += uniformpriorterm;
	if(debug == 1)printf("End Like: %.10g \n", lnew);
	//printf("End Like: %.10g \n", lnew);
	//}


       	//gettimeofday(&tval_after, NULL);

       	//timersub(&tval_after, &tval_before, &tval_resulttwo);
       	//printf("Time elapsed: %ld.%06ld %ld.%06ld\n", (long int)tval_resultone.tv_sec, (long int)tval_resultone.tv_usec, (long int)tval_resulttwo.tv_sec, (long int)tval_resulttwo.tv_usec);
	
	return lnew;
	//sleep(5);
	//return bluff;
	//sleep(5);

}

void  WriteSubIntStocProfLike(std::string longname, int &ndim){



        double *Cube = new double[ndim];
        int number_of_lines = 0;

        std::ifstream checkfile;
	std::string checkname;
	if(((MNStruct *)globalcontext)->sampler == 0){
	        checkname = longname+"phys_live.points";
	}
        if(((MNStruct *)globalcontext)->sampler == 1){
                checkname = longname+"_phys_live.txt";
        }
	//printf("%i %s \n", ((MNStruct *)globalcontext)->sampler, checkname.c_str());
        checkfile.open(checkname.c_str());
        std::string line;
        while (getline(checkfile, line))
                ++number_of_lines;

        checkfile.close();

        std::ifstream summaryfile;
	std::string fname;
	if(((MNStruct *)globalcontext)->sampler == 0){
		fname = longname+"phys_live.points";
	}
	if(((MNStruct *)globalcontext)->sampler == 1){
	        fname = longname+"_phys_live.txt";
	}

        summaryfile.open(fname.c_str());



        printf("Getting ML \n");
        double maxlike = -1.0*pow(10.0,10);
        for(int i=0;i<number_of_lines;i++){

                std::string line;
                getline(summaryfile,line);
                std::istringstream myStream( line );
                std::istream_iterator< double > begin(myStream),eof;
                std::vector<double> paramlist(begin,eof);
                double like = paramlist[ndim];

                if(like > maxlike){
                        maxlike = like;
                         for(int i = 0; i < ndim; i++){
                                 Cube[i] = paramlist[i];
                         }
                }

        }
        summaryfile.close();
	printf("ML val: %.10g \n", maxlike);

	int debug = 0;

	if(debug == 1)printf("In like \n");

	double *EFAC;
	double *EQUAD;
	long double LDparams;
	int pcount;

	double uniformpriorterm = 0;


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Timing Model////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	LDparams=Cube[0]*(((MNStruct *)globalcontext)->LDpriors[0][1]) + (((MNStruct *)globalcontext)->LDpriors[0][0]);
	
	pcount=0;
	long double phase = LDparams*((MNStruct *)globalcontext)->ReferencePeriod/SECDAY;
	pcount++;
	
	if(debug == 1)printf("Phase: %g \n", (double)phase);


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Form SATS///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	for(int p=0;p< ((MNStruct *)globalcontext)->pulse->nobs; p++){
		((MNStruct *)globalcontext)->pulse->obsn[p].sat = ((MNStruct *)globalcontext)->pulse->obsn[p].origsat-phase;
	}


	for(int p=0;p<((MNStruct *)globalcontext)->numFitJumps;p++){
		((MNStruct *)globalcontext)->pulse->jumpVal[((MNStruct *)globalcontext)->TempoJumpNums[p]]= 0;
	}


	fastformBatsAll(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars);       /* Form Barycentric arrival times */
	formResiduals(((MNStruct *)globalcontext)->pulse,((MNStruct *)globalcontext)->numberpulsars,0);       /* Form residuals */


	if(debug == 1)printf("Formed Residuals \n");

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////White noise/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	  
	if(((MNStruct *)globalcontext)->numFitEFAC == 0){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC == 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EFAC[o]=pow(10.0,Cube[pcount]);
		}
		if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[0]);}
		pcount++;
		
	}
	else if(((MNStruct *)globalcontext)->numFitEFAC > 1){
		EFAC=new double[((MNStruct *)globalcontext)->systemcount];
		for(int p=0;p< ((MNStruct *)globalcontext)->systemcount; p++){
			EFAC[p]=pow(10.0,Cube[pcount]);
			if(((MNStruct *)globalcontext)->EFACPriorType ==1) {uniformpriorterm += log(EFAC[p]);}
			pcount++;
		}
	}	
	  
	if(((MNStruct *)globalcontext)->numFitEQUAD == 0){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD == 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->numFitEQUAD > 1){
		EQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			EQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}


	double *SQUAD;
	if(((MNStruct *)globalcontext)->incShannonJitter == 0){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=0;
		}
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter == 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
		}
		pcount++;
	}
	else if(((MNStruct *)globalcontext)->incShannonJitter > 1){
		SQUAD=new double[((MNStruct *)globalcontext)->systemcount];
		for(int o=0;o<((MNStruct *)globalcontext)->systemcount; o++){
			SQUAD[o]=pow(10.0,Cube[pcount]);
			pcount++;
		}
	}	  
	  

	//printf("End of White\n");


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


	 long double **ProfileBats=new long double*[((MNStruct *)globalcontext)->pulse->nobs];
	 long double *ModelBats = new long double[((MNStruct *)globalcontext)->pulse->nobs];
	 for(int i = 0; i < ((MNStruct *)globalcontext)->pulse->nobs; i++){
	      //printf("badcorr? %i %g %g \n", i,  (double)((MNStruct *)globalcontext)->pulse->obsn[i].batCorr, (double)((MNStruct *)globalcontext)->pulse->obsn[i].residual);
	      int Nbin  = (int)((MNStruct *)globalcontext)->ProfileInfo[i][1];
	      ProfileBats[i] = new long double[Nbin];
	      for(int j = 0; j < Nbin; j++){
		    ProfileBats[i][j] = ((MNStruct *)globalcontext)->ProfileData[i][j][0] + ((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[0].batCorr;
		
	      }
	      
	      
	      ModelBats[i] = ((MNStruct *)globalcontext)->ProfileInfo[i][5]+((MNStruct *)globalcontext)->pulse->obsn[i].batCorr - ((MNStruct *)globalcontext)->pulse->obsn[0].batCorr - phase - ((MNStruct *)globalcontext)->pulse->obsn[i].residual/SECDAY ;
	 }


	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;

	double shapecoeff[numcoeff];
	double StocProfCoeffs[numshapestoccoeff];

	for(int i =0; i < numcoeff; i++){
		shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
	}
	double beta = ((MNStruct *)globalcontext)->MeanProfileBeta*((MNStruct *)globalcontext)->ReferencePeriod;


	for(int i =0; i < numshapestoccoeff; i++){
		StocProfCoeffs[i]= pow(10.0, 2*Cube[pcount]);
		pcount++;
	}

	if(numcoeff+1>=numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}


/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	double modelflux=0;
	    for(int j =0; j < numcoeff; j=j+2){
		modelflux+=sqrt(sqrt(M_PI))*sqrt(beta)*pow(2.0, 0.5*(1.0-j))*sqrt(((MNStruct *)globalcontext)->Binomial[j])*shapecoeff[j];
	   }

	double lnew = 0;
	int badshape = 0;

	int minoffpulse=750;
	int maxoffpulse=1250;
       // int minoffpulse=200;
       // int maxoffpulse=800;
	
	for(int t = 0; t < ((MNStruct *)globalcontext)->pulse->nobs; t++){
		if(debug == 1)printf("In toa %i \n", t);
		int nTOA = t;

		double detN  = 0;
		double Chisq  = 0;
		double logMargindet = 0;
		double Marginlike = 0;	 
		double JitterPrior = 0;	   

		double profilelike=0;

		long double FoldingPeriod = ((MNStruct *)globalcontext)->ProfileInfo[nTOA][0];
		long double FoldingPeriodDays = FoldingPeriod/SECDAY;
		int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[nTOA][1];
		double Tobs = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][2];
		double noiseval = (double)((MNStruct *)globalcontext)->ProfileInfo[nTOA][3];
		long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;

		double *Betatimes = new double[Nbins];
		double **AllHermiteBasis =  new double*[Nbins];
		double **JitterBasis  =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];

		for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
		for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){JitterBasis[i][j]=0;}}
	
	


		double noisemean=0;
		for(int j =minoffpulse; j < maxoffpulse; j++){
			noisemean += (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1];
		}

		noisemean = noisemean/(maxoffpulse-minoffpulse);

		double MLSigma = 0;
		int MLSigmaCount = 0;
		double dataflux = 0;
		for(int j = 0; j < Nbins; j++){
			if(j>=minoffpulse && j < maxoffpulse){
				double res = (double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] -noisemean;
				MLSigma += res*res; MLSigmaCount += 1;
			}
			else{
				if((double)(((MNStruct *)globalcontext)->ProfileData[nTOA][j][1] - noisemean)/MLSigma > 1){
					dataflux +=((double)((MNStruct *)globalcontext)->ProfileData[nTOA][j][1]-noisemean)*double(ProfileBats[t][1] - ProfileBats[t][0])*60*60*24;
				}
			}
		}

		MLSigma = sqrt(MLSigma/MLSigmaCount);
		double maxamp = dataflux/modelflux;

		if(debug == 1)printf("noise %g, dataflux %g, modelflux %g, maxamp %g \n", MLSigma, dataflux, modelflux, maxamp);


		long double binpos = ModelBats[nTOA];


		if(binpos < ProfileBats[nTOA][0])binpos+=FoldingPeriodDays;

		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < ProfileBats[nTOA][0])minpos=ProfileBats[nTOA][0];
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos> ProfileBats[nTOA][Nbins-1])maxpos =ProfileBats[nTOA][Nbins-1];


	    
		for(int j =0; j < Nbins; j++){
			long double timediff = 0;
			long double bintime = ProfileBats[t][j];
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;

			Betatimes[j]=timediff/beta;
			TNothpl(maxshapecoeff,Betatimes[j],AllHermiteBasis[j]);

			for(int k =0; k < maxshapecoeff; k++){
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));
				AllHermiteBasis[j][k]=AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatimes[j]*Betatimes[j]);

				if(k<numcoeff){ Hermitepoly[j][k] = AllHermiteBasis[j][k];}
			
			}

			JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1]);
			for(int k =1; k < numcoeff; k++){
				JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1] - sqrt(double(k+1))*AllHermiteBasis[j][k+1]);
			}	
	    }


	    double *shapevec  = new double[Nbins];
	    double *Jittervec = new double[Nbins];
	
	    double OverallProfileAmp = shapecoeff[0];
	    
	    shapecoeff[0] = 1;

	    dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
	    dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');
	
	    double *NDiffVec = new double[Nbins];

	    double maxshape=0;


	    for(int j =0; j < Nbins; j++){

		    if(shapevec[j] > maxshape){ maxshape = shapevec[j];}

	    }
	
	///////////////////////////////////////////Marginalise over arbitrary offset and absolute amplitude////////////////////////////////////////////////////////////


		double **M = new double*[Nbins];
		double **NM = new double*[Nbins];

		int Msize = 2+numshapestoccoeff;
		int Mcounter=0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			Msize++;
		}


		for(int i =0; i < Nbins; i++){
			Mcounter=0;
			M[i] = new double[Msize];
			NM[i] = new double[Msize];

			M[i][0] = 1;
			M[i][1] = shapevec[i];

			Mcounter = 2;

			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				M[i][Mcounter] = Jittervec[i];
				Mcounter++;
			}			

			for(int j = 0; j < numshapestoccoeff; j++){
			    M[i][Mcounter+j] = AllHermiteBasis[i][j];
			}
		}


		double **MNM = new double*[Msize];
		for(int i =0; i < Msize; i++){
		    MNM[i] = new double[Msize];
		}

		      
		Chisq = 0;
		detN = 0;

	
 
		double highfreqnoise = SQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		double flatnoise = EFAC[((MNStruct *)globalcontext)->sysFlags[nTOA]];
		
		double *profilenoise = new double[Nbins];

		for(int i =0; i < Nbins; i++){
			Mcounter=2;
			double noise = MLSigma*MLSigma +  pow(highfreqnoise*maxamp*shapevec[i],2);
			if(shapevec[i] > maxshape*0.001){
				noise +=  pow(flatnoise*maxamp*maxshape,2);
			}
			detN += log(noise);
			profilenoise[i] = sqrt(noise);

			double datadiff =  (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1];

			NDiffVec[i] = datadiff/(noise);
		       

			Chisq += datadiff*datadiff/(noise);
	
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
				M[i][2] = M[i][2]*dataflux/modelflux/beta;
				Mcounter++;
			}
			
			for(int j = 0; j < numshapestoccoeff; j++){
				M[i][Mcounter+j] = M[i][Mcounter+j]*dataflux;

			}


			for(int j = 0; j < Msize; j++){
				NM[i][j] = M[i][j]/(noise);
			}


		}
		
			
		dgemm(M, NM , MNM, Nbins, Msize,Nbins, Msize, 'T', 'N');


		Mcounter=2;
		double JitterDet = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0){
			double profnoise = EQUAD[((MNStruct *)globalcontext)->sysFlags[nTOA]];
			profnoise = profnoise*profnoise;
			JitterDet = log(profnoise);
			MNM[2][2] += 1.0/profnoise;
			Mcounter++;
		}


		double StocProfDet = 0;
		for(int j = 0; j < numshapestoccoeff; j++){
			StocProfDet += log(StocProfCoeffs[j]);
			MNM[Mcounter+j][Mcounter+j] +=  1.0/StocProfCoeffs[j];
		}

		double *dNM = new double[Msize];
		double *TempdNM = new double[Msize];
		dgemv(M,NDiffVec,dNM,Nbins,Msize,'T');

		for(int i =0; i < Msize; i++){
			TempdNM[i] = dNM[i];
		}


		int info=0;
		double Margindet = 0;
		dpotrfInfo(MNM, Msize, Margindet, info);
		dpotrs(MNM, TempdNM, Msize);
			    
		logMargindet = Margindet;

		Marginlike = 0;
		for(int i =0; i < Msize; i++){
			Marginlike += TempdNM[i]*dNM[i];
		}


		double finaloffset = TempdNM[0];
		double finalamp = TempdNM[1];
		double finalJitter = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0)finalJitter = TempdNM[2];

		double *StocVec = new double[Nbins];
		
		for(int i =0; i < Nbins; i++){
			if(((MNStruct *)globalcontext)->numFitEQUAD > 0)Jittervec[i] = M[i][2];
			if(((MNStruct *)globalcontext)->numFitEQUAD == 0)Jittervec[i] = 0;
		}

		dgemv(M,TempdNM,shapevec,Nbins,Msize,'N');

		

		TempdNM[0] = 0;
		TempdNM[1] = 0;
		if(((MNStruct *)globalcontext)->numFitEQUAD > 0)TempdNM[2] = 0;
		

		dgemv(M,TempdNM,StocVec,Nbins,Msize,'N');

		std::ofstream profilefile;
		std::string ProfileName =  ((MNStruct *)globalcontext)->pulse->obsn[t].fname;
		std::string dname = longname+ProfileName+"-Profile.txt";
	
		profilefile.open(dname.c_str());
		double MLChisq = 0;
		for(int i =0; i < Nbins; i++){
			MLChisq += pow(((double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] - shapevec[i])/profilenoise[i], 2);
			profilefile << i << " " << std::setprecision(10) << (double)((MNStruct *)globalcontext)->ProfileData[nTOA][i][1] << " " << shapevec[i] << " " << profilenoise[i] << " " << finaloffset + finalamp*M[i][1] << " " << finalJitter*Jittervec[i] << " " << StocVec[i] << "\n";

		}
	    	profilefile.close();
		delete[] profilenoise;
		delete[] StocVec;
		
		profilelike = -0.5*(detN + Chisq + logMargindet + StocProfDet + JitterDet - Marginlike);
		lnew += profilelike;

		printf("Profile chisq and like: %g %g \n", MLChisq, profilelike);
//		printf("Like: %i %.15g %.15g %.15g %.15g %.15g %.15g %.15g\n", nTOA,lnew, detN, Chisq, logMargindet, Marginlike, StocProfDet , JitterDet);

		delete[] shapevec;
		delete[] Jittervec;
		delete[] NDiffVec;
		delete[] dNM;
		delete[] Betatimes;

		for (int j = 0; j < Nbins; j++){
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		    delete[] AllHermiteBasis[j];
		    delete[] M[j];
		    delete[] NM[j];
		}
		delete[] Hermitepoly;
		delete[] AllHermiteBasis;
		delete[] JitterBasis;
		delete[] M;
		delete[] NM;

		for (int j = 0; j < Msize; j++){
		    delete[] MNM[j];
		}
		delete[] MNM;
	
	
	}
	 
	 
	 
	 

	
	for(int j =0; j< ((MNStruct *)globalcontext)->pulse->nobs; j++){
	    delete[] ProfileBats[j];
	}
	delete[] ProfileBats;
	delete[] ModelBats;
	delete[] EFAC;
	delete[] EQUAD;
	delete[] SQUAD;


	lnew += uniformpriorterm;
	printf("End Like: %.10g \n", lnew);


}



void PreComputeShapelets(double ***StoredShapelets, double **InterpolatedMeanProfile, double **InterpolatedJitterProfile, long double finalInterpTime, int numtointerpolate, double MeanBeta){


	printf("In function %Lg %i %g \n", finalInterpTime, numtointerpolate, MeanBeta);

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profile Params//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	int maxshapecoeff = 0;
	int numcoeff=((MNStruct *)globalcontext)->numshapecoeff;
	int numshapestoccoeff = ((MNStruct *)globalcontext)->numshapestoccoeff;


	double shapecoeff[numcoeff];

	for(int i =0; i < numcoeff; i++){
		shapecoeff[i]=((MNStruct *)globalcontext)->MeanProfileShape[i];
	}

	double beta = MeanBeta*((MNStruct *)globalcontext)->ReferencePeriod;

	if(numcoeff+1>=numshapestoccoeff+1){
		maxshapecoeff=numcoeff+1;
	}
	if(numshapestoccoeff+1 > numcoeff+1){
		maxshapecoeff=numshapestoccoeff+1;
	}

/////////////////////////////////////////////////////////////////////////////////////////////  
/////////////////////////Profiles////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

	int Nbins = (int)((MNStruct *)globalcontext)->ProfileInfo[0][1];
	long double ReferencePeriod = ((MNStruct *)globalcontext)->ReferencePeriod;
	long double FoldingPeriodDays = ReferencePeriod/SECDAY;
	double maxshapeamp = 0;

	printf("Computing %i Interpolated Profiles \n", numtointerpolate);
	for(int t = 0; t < numtointerpolate; t++){



		double **JitterBasis  =  new double*[Nbins];
		double **Hermitepoly =  new double*[Nbins];
		double **AllHermiteBasis = new double*[Nbins];

                for(int i =0;i<Nbins;i++){AllHermiteBasis[i]=new double[maxshapecoeff];for(int j =0;j<maxshapecoeff;j++){AllHermiteBasis[i][j]=0;}}
		for(int i =0;i<Nbins;i++){Hermitepoly[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){Hermitepoly[i][j]=0;}}
		for(int i =0;i<Nbins;i++){JitterBasis[i]=new double[numcoeff];for(int j =0;j<numcoeff;j++){JitterBasis[i][j]=0;}}

		long double interpStep = finalInterpTime;
		long double binpos = t*interpStep/SECDAY;

//		printf("Interp step %i %.10Lg %.10Lg \n", t, ReferencePeriod/Nbins, binpos);
		
		long double minpos = binpos - FoldingPeriodDays/2;
		if(minpos < 0 ) minpos= 0;
		long double maxpos = binpos + FoldingPeriodDays/2;
		if(maxpos > FoldingPeriodDays)maxpos = FoldingPeriodDays;
	    
		for(int j =0; j < Nbins; j++){
			long double timediff = 0;
			long double bintime = j*FoldingPeriodDays/Nbins;
			if(bintime  >= minpos && bintime <= maxpos){
			    timediff = bintime - binpos;
			}
			else if(bintime < minpos){
			    timediff = FoldingPeriodDays+bintime - binpos;
			}
			else if(bintime > maxpos){
			    timediff = bintime - FoldingPeriodDays - binpos;
			}

			timediff=timediff*SECDAY;
			double Betatime = timediff/beta;
			TNothpl(maxshapecoeff,Betatime,AllHermiteBasis[j]);

			for(int k =0; k < maxshapecoeff; k++){
				double Bconst=(1.0/sqrt(beta))/sqrt(pow(2.0,k)*sqrt(M_PI));
				AllHermiteBasis[j][k] = AllHermiteBasis[j][k]*Bconst*exp(-0.5*Betatime*Betatime);
				//if(t == 4847 && k == 0)printf("%.10Lg %.10Lg %g %i %i %g \n", timediff, binpos*SECDAY, Betatime, j, k, StoredShapelets[t][j][k]);

				if(k<numcoeff){ Hermitepoly[j][k] = AllHermiteBasis[j][k];}
			}


			JitterBasis[j][0] = (1.0/sqrt(2.0))*(-1.0*AllHermiteBasis[j][1]);
			for(int k =1; k < numcoeff; k++){
				JitterBasis[j][k] = (1.0/sqrt(2.0))*(sqrt(double(k))*AllHermiteBasis[j][k-1] - sqrt(double(k+1))*AllHermiteBasis[j][k+1]);
			}

			for(int k = 0; k < numshapestoccoeff; k++){
				StoredShapelets[t][j][k] = AllHermiteBasis[j][k];
			}	
	    	}

		double *shapevec  = new double[Nbins];
		double *Jittervec = new double[Nbins];


		dgemv(Hermitepoly,shapecoeff,shapevec,Nbins,numcoeff,'N');
		dgemv(JitterBasis,shapecoeff,Jittervec,Nbins,numcoeff,'N');

		
		for(int j =0; j < Nbins; j++){
			InterpolatedMeanProfile[t][j] = shapevec[j];
			InterpolatedJitterProfile[t][j] = Jittervec[j];
			if(shapevec[j] > maxshapeamp){ maxshapeamp = shapevec[j]; }
		}	


		delete[] shapevec;
		delete[] Jittervec;


		for (int j = 0; j < Nbins; j++){
		    delete[] AllHermiteBasis[j]; 
		    delete[] Hermitepoly[j];
		    delete[] JitterBasis[j];
		}
		delete[] AllHermiteBasis;
		delete[] Hermitepoly;
		delete[] JitterBasis;

	}
	printf("Finished Computing Interpolated Profiles, max amp is %g \n", maxshapeamp);

}

