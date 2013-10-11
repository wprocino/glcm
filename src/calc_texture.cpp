#include <RcppArmadillo.h>
using namespace arma;

// Define a pointer to a texture function that will be used to map selected 
// co-occurrence statistics to the below texture calculation functions.
typedef double (*pfunc)(mat, mat, mat, double, double);

double text_mean(mat pij, mat imat, mat jmat, double mr, double mc) {
    //Rcpp::Rcout << "text_mean" << std::endl;
    // Defined as in Lu and Batistella, 2005, page 252
    return(mr);
}

double text_variance(mat pij, mat imat, mat jmat, double mr, double mc) {
    //Rcpp::Rcout << "variance " << std::endl;
    // Defined as in Haralick, 1973, page 619 (equation 4)
    return(accu(pij % square((imat - mr))));
}

double text_covariance(mat pij, mat imat, mat jmat, double mr, double mc) {
    //Rcpp::Rcout << "covariance" << std::endl;
    // Defined as in Pratt, 2007, page 540
    return(accu((imat - mr) % (jmat - mc) % pij));
}

double text_homogeneity(mat pij, mat imat, mat jmat, double mr, double mc) {
    //Rcpp::Rcout << "homogeneity" << std::endl;
    // Defined as in Gonzalez and Woods, 2009, page 832
    return(accu(pij / (1 + abs(imat - jmat))));
}

double text_contrast(mat pij, mat imat, mat jmat, double mr, double mc) {
    //Rcpp::Rcout << "contrast" << std::endl;
    // Defined as in Gonzalez and Woods, 2009, page 832
    return(accu(square((imat - jmat)) % pij));
}

double text_dissimilarity(mat pij, mat imat, mat jmat, double mr, double mc) {
    //Rcpp::Rcout << "dissimilarity" << std::endl;
    //TODO: Find source for dissimilarity
    return(accu(pij % abs(imat - jmat)));
}

double text_entropy(mat pij, mat imat, mat jmat, double mr, double mc) {
    //Rcpp::Rcout << "entropy" << std::endl;
    // Defined as in Haralick, 1973, page 619 (equation 9)
    return(-accu(pij % log(pij + .00000001)));
}

double text_second_moment(mat pij, mat imat, mat jmat, double mr, double mc) {
    //Rcpp::Rcout << "second_moment" << std::endl;
    // Defined as in Haralick, 1973, page 619
    return(accu(square(pij)));
}

double text_correlation(mat pij, mat imat, mat jmat, double mr, double mc) {
    //Rcpp::Rcout << "correlation" << std::endl;
    // Defined as in Gonzalez and Woods, 2009, page 832
    // Calculate sig2r and sig2c (measures of row and column variance)
    double sig2c=0, sig2r=0;
    sig2r = sum(trans(square(linspace<vec>(1, pij.n_rows, pij.n_rows) - mr)) % sum(pij, 0));
    sig2c = sum(square(linspace<vec>(1, pij.n_cols, pij.n_cols) - mc) % sum(pij, 1));
    return(accu(((imat - mr) % (jmat - mc) % pij) / (sqrt(sig2r) * sqrt(sig2c))));
    //Rcpp::Rcout << "finished correlation" << std::endl;
}

//' Calculates a glcm texture for use in the glcm.R script
//' @export
// [[Rcpp::export]]
Rcpp::NumericVector calc_texture(arma::mat Rast,
        Rcpp::CharacterVector statistics, arma::vec base_indices,
        arma::vec offset_indices, int n_grey) {
    mat G(n_grey, n_grey, fill::zeros);
    mat pij(n_grey, n_grey, fill::zeros);
    mat imat(n_grey, n_grey, fill::zeros);
    mat jmat(n_grey, n_grey, fill::zeros);
    Rcpp::NumericVector textures(statistics.size());
    double mr=0, mc=0;

    std::map<std::string, double (*)(mat, mat, mat, double, double)> stat_func_map;
    stat_func_map["mean"] = text_mean;
    stat_func_map["variance"] = text_variance;
    stat_func_map["covariance"] = text_covariance;
    stat_func_map["homogeneity"] = text_homogeneity;
    stat_func_map["contrast"] = text_contrast;
    stat_func_map["dissimilarity"] = text_dissimilarity;
    stat_func_map["entropy"] = text_entropy;
    stat_func_map["second_moment"] = text_second_moment;
    stat_func_map["correlation"] = text_correlation;

    for(unsigned i=0; i < offset_indices.size(); i++) {
        // Subtract one from the below indices to correct for row and col 
        // indices starting at 0 in C++ versus 1 in R.
        G(Rast(base_indices(i) - 1) - 1, Rast(offset_indices(i) - 1) - 1)++;
    }
    pij = G / accu(G);

    // Make a matrix of i's and a matrix of j's to be used in the below matrix 
    // calculations. These matrices are the same shape as pij with the entries 
    // equal to the i indices of each cell (for the imat matrix, which is 
    // indexed over the rows) or the j indices of each cell (for the jmat 
    // matrix, which is indexed over the columns). Note that linspace<mat> 
    // makes a column vector.
    imat = repmat(linspace<vec>(1, G.n_rows, G.n_rows), 1, G.n_cols);
    jmat = trans(imat);
    // Calculate mr and mc (forms of col and row means), see Gonzalez and 
    // Woods, 2009, page 832
    mr = sum(trans(linspace<vec>(1, G.n_rows, G.n_rows)) % sum(pij, 0));
    mc = sum(linspace<vec>(1, G.n_cols, G.n_cols) % sum(pij, 1));

    // Loop over the selected statistics, using the stat_func_map map to map 
    // each selected statistic to the appropriate texture function.
    for(unsigned i=0; i < stat_func_map.size(); i++) {
        pfunc f = stat_func_map[Rcpp::as<std::string>(statistics(i))];
        textures(i) = (*f)(pij, imat, jmat, mr, mc);
    }

    return(textures);
}
