# Requires python3.6
import sys

import numpy as np
import seaborn as sns
import pandas as pd
# See: 
# http://pandas.pydata.org/pandas-docs/stable/generated/pandas.read_csv.html
# http://pandas.pydata.org/pandas-docs/stable/dsintro.html#console-display

def Select(df, SensorName ):
	mask 	= df.loc[:, 'Sensor'] == SensorName
	t 		= df.loc[mask];

	return t

# Extract mean and std according segmentation of the column HEADING:
# 1) get range of values with the given Heading.
# 2) extract the data and compute mean and std
# 3) return the values

def ExtractAccording(df, Heading):
    tmp = np.array( df.loc[:, Heading] )
    l = np.min(tmp)
    r = np.max(tmp)
    cnt, bin_boarders = np.histogram( tmp, range(l,r+2))
    bins = bin_boarders[:-1]
    #     print( len( cnt ) )
    #     print( len( bins ))
    #     print( cnt )
    #     print( bins )
    meanV = np.zeros(len(cnt))
    stdV = np.zeros(len(cnt))

    for i in range(len(cnt)):
        mask 	= df.loc[:, Heading] == bins[i]
        tt 		= df.loc[mask]
        # print( tt ) 
        # print( np.mean( np.array( tt.Value ) ))
        meanV[i] = np.mean( np.array( tt.Value ) )
        stdV[i] = np.std( np.array( tt.Value ) )

    # print( meanV )
    # print( stdV )
    return cnt, bins, meanV, stdV
    

def run( InFile, OUTDIR ):
  ## HEADER STRING = dIdx;pIdx;rIdx;Sensor;Value
  df = pd.read_csv( InFile, sep=';')
  df.info()

  print('Number of NANs: %i ' % df.isnull().sum().sum())

  # OUTDIR = 'out/'
  ############################################################################################################
  ### TIME 
  ############################################################################################################
  t = Select(df, 'TIME_WALL')
  #t = Select(df, ' ::mysys_node0_ame1_PWR250US')

  cnt, bins, meanV, stdV = ExtractAccording(t, 'pIdx')
  # print( cnt )
  # print( bins )
  # print( meanV )
  # print( stdV )

  g = sns.factorplot( x="pIdx", y="Value",
                     	data=t, saturation=.5,
                      kind="bar", aspect=2)

  (g.set_ylabels("Wall Time [ms]"))
  g.axes[0][0].set_yscale('log')

  # Save graphic
  FileName = OUTDIR + "WallTime.pdf"
  print('Save file: %s' % FileName )
  g.savefig(FileName)
  # Save data
  x = np.array([cnt, bins, meanV, stdV])
  fullOut = np.array([cnt, bins, meanV, stdV])
  x = np.transpose(x)
  print(x)
  # print(x.shape )
  FileName = OUTDIR + "WallTime.csv"
  print('Save file: %s' % FileName )
  np.savetxt(FileName, x, delimiter='; ', newline='\n', header='cnt, bins, mean, std') 
  # sns.plt.show()

  ############################################################################################################
  ### POWER AME 0 
  ############################################################################################################
  t = Select(df, '::mysys_node0_ame0_PWR250US')

  cnt, bins, meanV, stdV = ExtractAccording(t, 'pIdx')


  g = sns.factorplot( x="pIdx", y="Value",
                     	data=t, saturation=.5,
                      kind="bar", aspect=2)

  (g.set_ylabels("Average Power [W]"))
  # g.axes[0][0].set_yscale('log')

  # Save graphic
  FileName = OUTDIR + "Power_ame0.pdf"
  print('Save file: %s' % FileName )
  g.savefig(FileName)
  # Save data
  x = np.array([cnt, bins, meanV, stdV])
  fullOut = np.concatenate([fullOut, [meanV, stdV]], axis=0)

  x = np.transpose(x)
  print(x)
  # print(x.shape )
  FileName = OUTDIR + "Power_ame0.csv"
  print('Save file: %s' % FileName )
  np.savetxt(FileName, x, delimiter='; ', newline='\n', header='cnt, bins, mean, std') 
  # sns.plt.show()

  ############################################################################################################
  ### POWER AME 1 
  ############################################################################################################
  t = Select(df, ' ::mysys_node0_ame1_PWR250US')

  cnt, bins, meanV, stdV = ExtractAccording(t, 'pIdx')


  g = sns.factorplot( x="pIdx", y="Value",
                     	data=t, saturation=.5,
                      kind="bar", aspect=2)

  (g.set_ylabels("Average Power [W]"))
  # g.axes[0][0].set_yscale('log')

  # Save graphic
  FileName = OUTDIR + "Power_ame1.pdf"
  print('Save file: %s' % FileName )
  g.savefig(FileName)
  # Save data
  x = np.array([cnt, bins, meanV, stdV])
  fullOut = np.concatenate([fullOut, [meanV, stdV]], axis=0)

  x = np.transpose(x)
  print(x)
  # print(x.shape )
  FileName = OUTDIR + "Power_ame1.csv"
  print('Save file: %s' % FileName )
  np.savetxt(FileName, x, delimiter='; ', newline='\n', header='cnt, bins, mean, std') 
  # sns.plt.show()

  ############################################################################################################
  ### POWER TOTAL (0+1) 
  ############################################################################################################
  t0 = Select(df, '::mysys_node0_ame0_PWR250US')
  t1 = Select(df, ' ::mysys_node0_ame1_PWR250US')

  tmp0 = np.array(t0['Value'])
  tmp1 = np.array(t1['Value'])
  d = { 'pIdx': np.array(t0['pIdx']),
         'Value': tmp0 + tmp1}
  t = pd.DataFrame(d, index=range(len(t0)))


  cnt, bins, meanV, stdV = ExtractAccording(t, 'pIdx')


  g = sns.factorplot( x="pIdx", y="Value",
                     	data=t, saturation=.5,
                      kind="bar", aspect=2)

  (g.set_ylabels("Average Power [W]"))
  # g.axes[0][0].set_yscale('log')

  # Save graphic
  FileName = OUTDIR + "Power_Tot.pdf"
  print('Save file: %s' % FileName )
  g.savefig(FileName)
  # Save data
  x = np.array([cnt, bins, meanV, stdV])
  fullOut = np.concatenate([fullOut, [meanV, stdV]], axis=0)

  x = np.transpose(x)
  print(x)
  # print(x.shape )
  FileName = OUTDIR + "Power_Tot.csv"
  print('Save file: %s' % FileName )
  np.savetxt(FileName, x, delimiter='; ', newline='\n', header='cnt, bins, mean, std') 
  # sns.plt.show()

  ############################################################################################################
  ### Energy TOTAL (0+1) 
  ############################################################################################################
  t0 = Select(df, '::mysys_node0_ame0_PWR250US')
  t1 = Select(df, ' ::mysys_node0_ame1_PWR250US')
  ttime = Select(df, 'TIME_WALL')

  tmp0 = np.array(t0['Value'])
  tmp1 = np.array(t1['Value'])
  tmp3 = np.array(ttime['Value'])

  d = { 'pIdx': np.array(t0['pIdx']),
         'Value': (tmp0 + tmp1)*tmp3 / 1000}
  t = pd.DataFrame(d, index=range(len(t0)))

  cnt, bins, meanV, stdV = ExtractAccording(t, 'pIdx')


  g = sns.factorplot( x="pIdx", y="Value",
                     	data=t, saturation=.5,
                      kind="bar", aspect=2)

  (g.set_ylabels("Total Energy [J]"))
  g.axes[0][0].set_yscale('log')

  # Save graphic
  FileName = OUTDIR + "Energy_Tot.pdf"
  print('Save file: %s' % FileName )
  g.savefig(FileName)
  # Save data
  x = np.array([cnt, bins, meanV, stdV])
  fullOut = np.concatenate([fullOut, [meanV, stdV]], axis=0)

  x = np.transpose(x)
  print(x)
  # print(x.shape )
  FileName = OUTDIR + "Energy_Tot.csv"
  print('Save file: %s' % FileName )
  np.savetxt(FileName, x, delimiter='; ', newline='\n', header='cnt, bins, mean, std') 
  # sns.plt.show()


  ### FULL OUT
  fullOut = np.transpose(fullOut)
  print(fullOut)
  # print(x.shape )
  FileName = OUTDIR + "AllDataOut.csv"
  print('Save file: %s' % FileName )
  np.savetxt(FileName, fullOut, delimiter='; ', newline='\n', header='cnt, bins, Time mean, Time std, Power 0 mean, Power 0 std, Power 1 mean, Power 1 std, Power Tot mean, Power Tot std, Energy Tot mean, Energy Tot std,') 

def usage():
  print(" Generates Time / Power / Energy Plots for MBs")
  print("Usage: %s <LongCSV input file> <OUT PATH>" % sys.argv[0])
  print()

if __name__ == '__main__':
    if ('--help' in sys.argv) or ('-h' in sys.argv):
      usage()

    else:
        kwargs = {}
        if len(sys.argv) != 3:
          usage()
        else:
          run( sys.argv[1], sys.argv[2] )