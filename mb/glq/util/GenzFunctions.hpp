#include <cmath>

#ifndef PI
	#define PI 3.1415926535897932384626433832795028841971693993751
#endif
#ifndef FABS
	#define FABS(a) ((a)>=0?(a):-(a))
#endif

// SOURCE OF GENZ-FUNCTION
// BOOK: NumericalIntegreation, page 338
// Parameters u in [0,1] should not affect the difficulty of the integral
// Parameter a,  increasing norm(a) generally makes the problem of integration more difficult.

///////////////////////////////////////////////////////
// DEFINE THE SIX GENZ TESTFUNCTIONS FOR THE 1D CASE
///////////////////////////////////////////////////////
// CHECKED MATLAB CODE:
// f1 = @(x,u,a) cos(2*pi*u + a*x);
// f2 = @(x,u,a) 1./(a.^(-2) + (x-u).^2);
// f3 = @(x,u,a) (1 + a*x).^(-(1+1));
// f4 = @(x,u,a) exp( - a^2*(x-u).^2);
// f5 = @(x,u,a) exp( - a*abs(x-u) );
// f6 = @(x,u,a) exp(a*x).*(x<u);

// Oscillatory
template<typename T>
T f1( T x, T u, T a )
{
	return cos(2*PI*u + a*x);
}

// Product Peak
template<typename T>
T f2( T x, T u, T a )
{
	return 1/( 1/(a*a) + (x-u)*(x-u) );
}

// Corner Peak
template<typename T>
T f3( T x, T u, T a )
{
	return pow(1 + a*x, -2 );
}

// Gaussian
template<typename T>
T f4( T x, T u, T a )
{
	return exp( -(a*a)*(x-u)*(x-u));
}

// C0 function
template<typename T>
T f5( T x, T u, T a )
{
	return exp( -a*fabs(x-u));
}

// Discontinuous
template<typename T>
T f6( T x, T u, T a )
{
	if( x > u ) return 0;
	else return exp( a*x ); 
}

///////////////////////////////////////////////////////
// DEFINE THE ANALYTICAL INTEGRAL OF IT IN [0,1]
///////////////////////////////////////////////////////
// CHECKED MATLAB CODE:
// % closed form integral solution for interval [0,1] for 1Dim-Case
// f1Integral = @(u,a) sin(2*pi*u+a)/a - sin(2*pi*u)/a;
// f2Integral = @(u,a) (atan((1-u)*a) - atan(-u*a))*a; 
// f3Integral = @(u,a) 1/a - (1/(a*(1+a)));
// f4Integral = @(u,a) (sqrt(pi)/(2*a))*( erf((1-u)*a) - erf(-u*a) );
// f5Integral = @(u,a) (2 - exp(-a*u) - exp(a*(u-1)))/a;
// f6Integral = @(u,a) ( exp(a*u) - 1 )/a;

template<typename T>
T RefIntegral_f1( T u, T a )
{	
	return sin(2*PI*u+a)/a - sin(2*PI*u)/a;
}

template<typename T>
T RefIntegral_f2( T u, T a )
{	
	return (atan((1-u)*a) - atan(-u*a))*a; 
}

template<typename T>
T RefIntegral_f3( T u, T a )
{	
	return 1/a - (1/(a*(1+a)));
}

template<typename T>
T RefIntegral_f4( T u, T a )
{	
	return (sqrt(PI)/(2*a))*( erf((1-u)*a) - erf(-u*a) );
}

template<typename T>
T RefIntegral_f5( T u, T a )
{	
	return (2 - exp(-a*u) - exp(a*(u-1)))/a;
}

template<typename T>
T RefIntegral_f6( T u, T a )
{	
	return ( exp(a*u) - 1 )/a;
}

template<typename T>
T RefIntegeral( int FunctionId, T u, T a )
{
 	if( FunctionId == 1 ) 	   return RefIntegral_f1( u, a );
 	else if( FunctionId == 2 ) return RefIntegral_f2( u, a );
 	else if( FunctionId == 3 ) return RefIntegral_f3( u, a );
 	else if( FunctionId == 4 ) return RefIntegral_f4( u, a );
 	else if( FunctionId == 5 ) return RefIntegral_f5( u, a );
 	else if( FunctionId == 6 ) return RefIntegral_f6( u, a );
 	else
 	{
 		printf("FunctionId = %i not supported\n", FunctionId );
 		exit(-1);
 	}
}

template<typename T>
struct DataHandle_1D_Type
{
	T u;
	T a;
	int FunctionId;
};

template<typename T>
T f_handle_1d( T x, void* data_ptr )
{
 	DataHandle_1D_Type<T> *data = static_cast<DataHandle_1D_Type<T> *>( data_ptr ); 

 	// printf("[u,a,n]: %f %f %i\n", data->u, data->a, data->FunctionId );

 	if( data->FunctionId == 1 )  	 return f1( x, data->u, data->a );
 	else if( data->FunctionId == 2 ) return f2( x, data->u, data->a );
 	else if( data->FunctionId == 3 ) return f3( x, data->u, data->a );
 	else if( data->FunctionId == 4 ) return f4( x, data->u, data->a );
 	else if( data->FunctionId == 5 ) return f5( x, data->u, data->a );
 	else if( data->FunctionId == 6 ) return f6( x, data->u, data->a );
 	else
 	{
 		printf("data.FunctionId = %i not supported\n", data->FunctionId );
 		exit(-1);
 	}
}
