# Prefetch and Reuse-Aware Set Duelling: Selecting Cache Policies Beyond Miss Intensity

**Course:** CS683 : Advance Computer Architecture  
**Author:** Sourabh Patel  
**Email:** 26m0783@iitb.ac.in  
**Simulator:** ChampSim

## Overview

This project studies whether changing the policy-selection signal in Set Dueling improves cache replacement decisions. Conventional Set Dueling compares misses in leader sets to choose between two candidate replacement policies for follower sets. However, workload phases differ in prefetch behavior and data reuse, which miss intensity alone may not adequately capture.

The project compares three selection signals:

- **Miss intensity:** the conventional baseline.
- **Prefetch intensity:** a proposed signal reflecting the usefulness or pollution of prefetched lines in each leader group.
- **Reuse intensity:** a proposed signal reflecting how often lines in each leader group are reused before eviction.

Only the decision signal is intended to change. The candidate policies, leader/follower organization, and other Set Dueling mechanics should remain unchanged across comparisons.

## Objectives

1. Implement prefetch-based and reuse-based policy selection in ChampSim.
2. Compare both against conventional miss-based Set Dueling on identical workloads.
3. Measure IPC, LLC MPKI, and execution time.
4. Explain which signal works best for particular workloads or phases, including cases with no improvement or regressions.

## Checkpoint I: Baseline and Prefetch-Based Selection

- Set up ChampSim and choose two candidate policies, such as SRRIP and BRRIP.
- Run conventional miss-based Set Dueling on selected workloads.
- Record baseline IPC, LLC MPKI, and execution time.
- Modify the policy-selection counter to use prefetch intensity instead of miss intensity.
- Run the prefetch-based version with the prefetcher enabled on the same workloads.
- Compare its metrics with the miss-based baseline and identify workloads or phases that improve or regress.

The hypothesis is that prefetch behavior can change which replacement policy is preferable. A policy that performs well for demand accesses may behave differently when prefetches fill the cache. This is an expected insight to investigate, not an established result.

## Checkpoint II: Reuse-Based Selection and Comparison

- Modify the policy-selection counter to use reuse intensity while keeping the remaining mechanism unchanged.
- Run the reuse-based version on the same workloads.
- Collect IPC, LLC MPKI, and execution time.
- Compare miss-based, prefetch-based, and reuse-based Set Dueling per workload.
- Analyze which signal selects the better policy for different phases or workload types, and where the new signals do not help.

The hypothesis is that reuse information may help protect useful data, especially when streaming accesses and repeated accesses coexist. No signal is assumed to win universally.

## References

The following related works are listed in the project proposal:

1. Qureshi et al., “Adaptive Insertion Policies for High Performance Caching,” ISCA 2007.
2. Feng et al., “Dynamic Access Distance Driven Cache Replacement,” TACO 2011.
3. Patsidis and Vavouliotis, “Context-Aware Set Dueling for Dynamic Policy Arbitration,” IEEE Computer Architecture Letters, 2025.
