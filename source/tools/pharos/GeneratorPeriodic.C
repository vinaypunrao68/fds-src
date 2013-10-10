#include <assert.h>

#include "GeneratorPeriodic.H"
#include "Timer.H"

GeneratorPeriodic::GeneratorPeriodic(long period_in, long rate_in, long edt_in):
	rate(rate_in),	
	exp_dt(edt_in),
	random_period(0.1, 1.0),   //period changes from 100 ms to 1 sec
	random_ios(1.0, 15.0)		//number of ios per period changes from 1 IOs to 30 IOs/period
{
	rand_period = 0;
	period = period_in/1000000.0;
	last_time = 0.0;	
	period_start = 0.0;
	
	if (edt_in == 0) {
		rand_period = 1;
	}
	
	if (rand_period == 0)
	{
		//calculate number of I/Os per period based on expected disk time per I/O
		ios_per_period = (rate * period)/exp_dt;
		ios_left_in_period = ios_per_period;
		printf("GeneratorPeriodic: period %.6f, rate %ld, exp_dt %ld, %.2f I/Os per period\n", period, rate, exp_dt, ios_per_period);
	}
	else
	{
		//last_time = 0.25; //random_period.getDouble();
		double new_period = random_period.getDouble();;
		RandomUniform rios(1.0, new_period*30.0);
		ios_per_period = rios.getDouble();
		
		ios_left_in_period = (int)ios_per_period;
		//printf("GeneratorPeriodic: random period from 500 ms to 10 s, random IOs per period from 2 to 20\n start at time %.3f\n", last_time);			
	}
}

GeneratorPeriodic::GeneratorPeriodic(long period_in, long rate_in, int ios_in):
	random_period(0.5, 10.0),   //period changes from 500ms to 10 sec
	random_ios(2.0, 20.0)		//number of ios per period changes from 2 IOs to 20 IOs
{
	rate = rate_in;	
	period = period_in/1000000.0;
	last_time = 0.0;	
	period_start = 0.0;
	
	ios_per_period = (double) ios_in;
	ios_left_in_period = ios_per_period;
	exp_dt = (rate * period) / ios_per_period;
	printf("GeneratorPeriodic: period %.6f, rate %ld, exp_dt %ld, %.2f I/Os per period\n", period, rate, exp_dt, ios_per_period);
}

void GeneratorPeriodic::make(IO& io, double, double)
{
  int send_all = 1;
  rand_period = 0;
	
	//period changes from 500ms to 10 sec
	//number of ios per period changes from 2 IOs to 20 IOs
	
	if (send_all) {
		while (ios_left_in_period < 1) 
		{
			if (rand_period == 0) {
				last_time += period;
				ios_left_in_period += ios_per_period;
			}
			else {
				double new_period = random_period.getDouble();
				RandomUniform rios(1.0, new_period*30.0);
				double new_ios_per_period = rios.getDouble();
	
					
				//if (new_ios_per_period > 15)
				//	new_ios_per_period = 15;		
				
				last_time += new_period;
				ios_left_in_period += (int)new_ios_per_period;
				//printf("GeneratorPeriodic: period %.6f, %.2f I/Os per period, send @ %.6f\n", new_period, new_ios_per_period, last_time);				
			}			
		}
		
		io.setTime(last_time);
		io.setDeadline(last_time+period);

		//relative deadline
		//  if (last_time > now())
		//    io.setDeadline(last_time+period);
		//  else
		//  	io.setDeadline(now() + period);		
		
		--ios_left_in_period;
	}
	else {
    	io.setTime(last_time);
    	io.setDeadline(period_start + period);
    	--ios_left_in_period;  		
		
		last_time += (double)exp_dt / (double)rate;
		while (ios_left_in_period < 1) 
		{
			period_start += period;
			ios_left_in_period += ios_per_period;
		}				
	}	
}
