#include "StokesProblem.hpp"

#include <polyfem/StringUtils.hpp>

namespace polyfem
{
	DrivenCavity::DrivenCavity(const std::string &name)
	: Problem(name)
	{
		// boundary_ids_ = {1};
	}

	void DrivenCavity::rhs(const std::string &formulation, const Eigen::MatrixXd &pts,const double t, Eigen::MatrixXd &val) const
	{
		val = Eigen::MatrixXd::Zero(pts.rows(), pts.cols());
	}

	void DrivenCavity::bc(const Mesh &mesh, const Eigen::MatrixXi &global_ids, const Eigen::MatrixXd &uv, const Eigen::MatrixXd &pts,const double t, Eigen::MatrixXd &val) const
	{
		val = Eigen::MatrixXd::Zero(pts.rows(), pts.cols());

		for(long i = 0; i < pts.rows(); ++i)
		{
			if(mesh.get_boundary_id(global_ids(i))== 1)
				val(i, 1)=0.25;
			// else if(mesh.get_boundary_id(global_ids(i))== 3)
				// val(i, 1)=-0.25;
		}

		val *= t;
	}


	Flow::Flow(const std::string &name)
	: Problem(name)
	{
		boundary_ids_ = {1, 3, 7};
		inflow_ = 1;
		outflow_ = 3;
		flow_dir_ = 0;

		inflow_amout_ = 0.25;
		outflow_amout_ = 0.25;
	}

	void Flow::rhs(const std::string &formulation, const Eigen::MatrixXd &pts,const double t, Eigen::MatrixXd &val) const
	{
		val = Eigen::MatrixXd::Zero(pts.rows(), pts.cols());
	}

	void Flow::bc(const Mesh &mesh, const Eigen::MatrixXi &global_ids, const Eigen::MatrixXd &uv, const Eigen::MatrixXd &pts,const double t, Eigen::MatrixXd &val) const
	{
		val = Eigen::MatrixXd::Zero(pts.rows(), pts.cols());

		for(long i = 0; i < pts.rows(); ++i)
		{
			if(mesh.get_boundary_id(global_ids(i))== inflow_)
				val(i, flow_dir_)=inflow_amout_;
			else if(mesh.get_boundary_id(global_ids(i))== outflow_)
				val(i, flow_dir_)=outflow_amout_;
		}

		val *= t;
	}

	void Flow::set_parameters(const json &params)
	{
		if(params.find("inflow") != params.end())
		{
			inflow_ = params["inflow"];
		}

		if(params.find("outflow") != params.end())
		{
			outflow_ = params["outflow"];
		}

		if(params.find("inflow_amout") != params.end())
		{
			inflow_amout_ = params["inflow_amout"];
		}

		if(params.find("outflow_amout") != params.end())
		{
			outflow_amout_ = params["outflow_amout"];
		}

		if(params.find("direction") != params.end())
		{
			flow_dir_ = params["direction"];
		}

		boundary_ids_.clear();

		if(params.find("obstacle") != params.end())
		{
			const auto obstacle = params["obstacle"];
			if(obstacle.is_array()){
				for(size_t k = 0; k < obstacle.size(); ++k){
					const auto tmp = obstacle[k];
					if(tmp.is_string())
					{
						const std::string tmps = tmp;
						const auto endings = StringUtils::split(tmps, ":");
						assert(endings.size() == 2);
						const int start = atoi(endings[0].c_str());
						const int end = atoi(endings[1].c_str());

						for(int i = start; i <= end; ++i)
							boundary_ids_.push_back(i);
					}
					else
						boundary_ids_.push_back(tmp);
				}
			}
		}

		boundary_ids_.push_back(inflow_);
		boundary_ids_.push_back(outflow_);

		std::sort(boundary_ids_.begin(), boundary_ids_.end());
   		auto it = std::unique(boundary_ids_.begin(), boundary_ids_.end());
   		boundary_ids_.resize(std::distance(boundary_ids_.begin(), it));

   		// for(int i : boundary_ids_)
   		// 	std::cout<<"i "<<i<<std::endl;
	}




	TimeDependentFlow::TimeDependentFlow(const std::string &name)
	: Flow(name)
	{ }

	void TimeDependentFlow::initial_solution(const Eigen::MatrixXd &pts, Eigen::MatrixXd &val) const
	{
		val = Eigen::MatrixXd::Zero(pts.rows(), pts.cols());
	}
}