# -*- coding: utf-8 -*-
"""
Created on Thu Nov 21 21:43:14 2019

@author: janse
"""
# Alias pandas package 
import pandas as pd
from matplotlib import pyplot as plt
import random
import numpy as np

# Read scooby data
df = pd.read_csv("SD_Show_s1e1.csv", header=None, names=['event', 'time'])

def dupicate_data(df):
    # Duplicate events randomly because SCIENCE
    for i in range(100):
        event_index = random.randint(0, len(df) - 1)
        
        # Different ways to access dataframe columns
        event = df['event'][event_index]
        recent_time = np.max(df.time)
        
        tm = random.randint(recent_time, recent_time + 10)
        
        # Append returns the concatination of the data frame and new row, does not change old df
        df = df.append({"event":event, "time":tm}, ignore_index=True)

    return df
    
# User numpy to count occurances of events
# Returns 2-d array [[event1, event2, ...], [event1_count, event2_count, ...]]
occurance_counts = np.unique(df.event, return_counts=True)

# plt.bar(occurance_counts[0], occurance_counts[1])
plt.scatter(df.time, df.event)
plt.show()
