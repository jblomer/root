## \file
## \ingroup tutorial_roofit
## \notebook
## Use of mixture models in RooFit.
##
## This tutorial shows, how to use mixture models for Likelihood Calculation in ROOT. Instead of directly
## calculating the likelihood we use simulation based inference (SBI) as shown in tutorial 'rf615_simulation_based_inference.py'.
## We train the classifier to discriminate between samples from an background hypothesis here the zz samples and a target
## hypothesis, here the higgs samples. The data preparation is based on the tutorial 'df106_HiggsToFourLeptons.py'.
##
## An introduction to mixture models can be found here https://arxiv.org/pdf/1506.02169.
##
## A short summary:
## We assume the whole probability distribution can be written as a mixture of several components, i.e.
## $$p(x|\theta)= \sum_{c}w_{c}(\theta)p_{c}(x|\theta)$$
## We can write the likelihood ratio in terms of pairwise classification problems
## \begin{align*}
##  \frac{p(x|\mu)}{p(x|0)}&= \frac{\sum_{c}w_{c}(\mu)p_{c}(x|\mu)}{\sum_{c'}w_{c'}(0)p_{c'}(x|0)}\\
##  &=\sum_{c}\Bigg[\sum_{c'}\frac{w_{c'}(0)}{w_{c}(\mu)}\frac{p_{c'}(x|0)}{p_{c}(x|\mu)}\Bigg]^{-1},
## \end{align*}
## where mu is the signal strength, and a value of 0 corresponds to the background hypothesis. Using this decomposition, one is able to use the pairwise likelihood ratios.
##
## Since the only free parameter in our case is mu, the distributions are independent of this parameter and the dependence on the signal strength can be encoded into the weights.
## Thus, the subratios simplify dramatically since they are independent of theta and these ratios can be pre-computed and the classifier does
## not need to be parametrized.
##
## If you wish to see an analysis done with template histograms see 'hf001_example.py'.
##
## \macro_image
## \macro_code
## \macro_output
##
## \date September 2024
## \author Robin Syring


import ROOT
import os
import numpy as np
import xgboost as xgb

# Get Dataframe from tutorial df106_HiggsToFourLeptons.py
# Adjust the path if running locally
df = ROOT.RDataFrame("tree", ROOT.gROOT.GetTutorialDir().Data() + "/dataframe/df106_HiggsToFourLeptons.root")

# Initialize a dictionary to store counts and weight sums for each category
results = {}


# Extract the relevant columns once and avoid repeated calls
data_dict = df.AsNumpy(columns=["m4l", "sample_category", "weight"])


weights_dict = {
    "data": data_dict["weight"][data_dict["sample_category"] == ["data"]].sum(),
    "zz": data_dict["weight"][data_dict["sample_category"] == ["zz"]].sum(),
    "other": data_dict["weight"][data_dict["sample_category"] == ["other"]].sum(),
    "higgs": data_dict["weight"][data_dict["sample_category"] == ["higgs"]].sum(),
}

# Loop over each sample category
for sample_category in ["data", "higgs", "zz", "other"]:

    weight_sum = weights_dict[sample_category]

    mask = data_dict["sample_category"] == sample_category
    # Normalize each weight
    weights = data_dict["weight"][mask]
    # Extract the weight_modified
    weight_modified = weights / weight_sum

    count = np.sum(mask)

    # Store the count and weight sum in the dictionary
    results[sample_category] = {
        "weight_sum": weight_sum,
        "weight_modified": weight_modified,
        "count": count,
        "weight": weights,
    }


# Extract the mass for higgs and zz
higgs_data = data_dict["m4l"][data_dict["sample_category"] == ["higgs"]]
zz_data = data_dict["m4l"][data_dict["sample_category"] == ["zz"]]


# Prepare sample weights
sample_weight_higgs = np.array([results["higgs"]["weight_modified"]]).flatten()
sample_weight_zz = np.array([results["zz"]["weight_modified"]]).flatten()

# Putting sample weights together in the same manner as the training data
sample_weight = np.concatenate([sample_weight_higgs, sample_weight_zz])

# For Training purposes we have to get rid of the negative weights, since xgb can't handle them
sample_weight[sample_weight < 0] = 1e-6

# Prepare the features and labels
X = np.concatenate((higgs_data, zz_data), axis=0).reshape(-1, 1)
y = np.concatenate([np.ones(len(higgs_data)), np.zeros(len(zz_data))])

# Train the Classifier to discriminate between higgs and zz
model_xgb = xgb.XGBClassifier(n_estimators=1000, max_depth=5, eta=0.2, min_child_weight=1 / 1000000)
model_xgb.fit(X, y, sample_weight=sample_weight)


# Prepare the observed data set
observed_data = data_dict["m4l"][data_dict["sample_category"] == ["data"]]

# Building a RooRealVar based on the observed data
m4l = ROOT.RooRealVar("m4l", "Four Lepton Invariant Mass", np.min(observed_data), np.max(observed_data))


# Define functions to compute the learned likelihood. One function is simply the vectorized version. The 'calculate_likelihood_xgb' is needed as input for the  'bindFunction' object.
def calculate_likelihood_xgb_vec(m4l_arr):
    prob = model_xgb.predict_proba(m4l_arr)[:, 0]
    return (1 - prob) / prob


def calculate_likelihood_xgb(m4l):
    m4l_arr = np.array([[m4l]])
    return calculate_likelihood_xgb_vec(m4l_arr)


llh = ROOT.RooFit.bindFunction(f"llh", calculate_likelihood_xgb, m4l)

# Plot the likelihood
frame1 = m4l.frame(Title=f"Likelihood", Range=(0, 200))
llh.plotOn(frame1, ShiftToZero=False)

# Number of signals and background
n_signal = results["higgs"]["weight"].sum()
n_back = results["zz"]["weight"].sum()


# Define weight functions
def weight_back(mu) -> float:
    return n_back / (n_back + mu * n_signal)


def weight_signal(mu) -> float:
    return 1 - weight_back(mu)


# Define the likelihood ratio accordingly to mixture models
def likelihood_ratio(mu, precomputed_likelihood):
    return 1 / (weight_back(mu) / weight_back(0) + weight_signal(mu) / weight_back(0) * precomputed_likelihood) + 1 / (
        weight_back(mu) / weight_signal(0) * 1 / precomputed_likelihood + weight_signal(mu) / weight_signal(0)
    )


# Weights for observed data are 1
nll_weights = 1.0
# Define the number of events for observed data
n_obs = len(observed_data)


# Extended likelihood term
def extended_term(mu, n):
    return (weight_back(mu)) ** n * np.exp(mu * n_signal)


# Calculate nll for all m4l values
def evaluate_nll(mu):
    likelihoods = likelihood_ratio(mu, precomputed_likelihoods)
    extended_val = np.log(extended_term(mu, n_obs))
    return np.sum(nll_weights * np.log(likelihoods)) + extended_val


# Precompute the likelihood values for all m4l values
m4l_values = observed_data.flatten()
precomputed_likelihoods = calculate_likelihood_xgb_vec(m4l_values)


mu_var = ROOT.RooRealVar("mu", "mu", 0.1, 5)
nll = ROOT.RooFit.bindFunction(f"nll", evaluate_nll, mu_var)

# Plot the nll computet by the mixture model
frame2 = mu_var.frame(Title="NLL")
nll.plotOn(frame2)

# Write the plots into one canvas
c = ROOT.TCanvas("rf618_sbi_higgs", "rf618_sbi_higgs", 800, 400)
c.Divide(2)
c.cd(1)
ROOT.gPad.SetLeftMargin(0.15)
frame1.GetYaxis().SetTitleOffset(1.8)
frame1.Draw()
c.cd(2)
ROOT.gPad.SetLeftMargin(0.15)
frame2.GetYaxis().SetTitleOffset(1.8)
frame2.Draw()


# Compute the minimum via minuit and display the results
minimizer = ROOT.RooMinimizer(nll)
minimizer.setErrorLevel(0.5)  # Adjust the error level in the minimization to work with likelihoods
minimizer.setPrintLevel(-1)
minimizer.minimize("Minuit2")
result = minimizer.save()
result.Print()
