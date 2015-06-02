/*************************************************************************
    > File Name: ILPColoring.h
    > Author: Yibo Lin
    > Mail: yibolin@utexas.edu
    > Created Time: Sat 23 May 2015 11:21:08 AM CDT
 ************************************************************************/

#ifndef LIMBO_ALGORITHMS_COLORING_ILPCOLORING
#define LIMBO_ALGORITHMS_COLORING_ILPCOLORING

#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <limits>
#include <cassert>
#include <cmath>
#include <stdlib.h>
#include <cstdio>
#include <sstream>
#include <algorithm>
#include <boost/cstdint.hpp>
#include <boost/unordered_map.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/subgraph.hpp>
#include <boost/property_map/property_map.hpp>
//#include <boost/graph/bipartite.hpp>
//#include <boost/graph/kruskal_min_spanning_tree.hpp>
//#include <boost/graph/prim_minimum_spanning_tree.hpp>
//#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <limbo/string/String.h>
#include <limbo/algorithms/coloring/Coloring.h>
#include "gurobi_c++.h"

namespace limbo { namespace algorithms { namespace coloring {

using std::ostringstream;
using boost::unordered_map;

template <typename GraphType>
class ILPColoring : public Coloring<GraphType>
{
	public:
		typedef Coloring<GraphType> base_type;
		using typename base_type::graph_type;
		using typename base_type::graph_vertex_type;
		using typename base_type::graph_edge_type;
		using typename base_type::vertex_iterator_type;
		using typename base_type::edge_iterator_type;
		using typename base_type::ColorNumType;
		/// edge weight is used to differentiate conflict edge and stitch edge 
		/// non-negative weight implies conflict edge 
		/// negative weight implies stitch edge 
		typedef typename boost::property_map<graph_type, boost::edge_weight_t>::type edge_weight_map_type;
		typedef typename boost::property_map<graph_type, boost::edge_weight_t>::const_type const_edge_weight_map_type;
		/// use vertex color to save vertex stitch candidate number 
		typedef typename boost::property_map<graph_type, boost::vertex_color_t>::type vertex_color_map_type;
		typedef typename boost::property_map<graph_type, boost::vertex_color_t>::const_type const_vertex_color_map_type;

		// hasher class for graph_edge_type
		struct edge_hash_type : std::unary_function<graph_edge_type, std::size_t>
		{
			std::size_t operator()(graph_edge_type const& e) const 
			{
				std::size_t seed = 0;
				boost::hash_combine(seed, e.m_source);
				boost::hash_combine(seed, e.m_target);
				return seed;
			}
		};

		/// constructor
		ILPColoring(graph_type const& g) 
			: base_type(g)
		{}
		/// destructor
		virtual ~ILPColoring() {}

	protected:
		/// \return objective value 
		virtual double coloring();
};

template <typename GraphType>
double ILPColoring<GraphType>::coloring()
{
	uint32_t vertex_num = boost::num_vertices(this->m_graph);
	uint32_t edge_num = boost::num_edges(this->m_graph);
	uint32_t vertex_variable_num = vertex_num<<1;

	unordered_map<graph_vertex_type, uint32_t> hVertexIdx; // vertex index 
	unordered_map<graph_edge_type, uint32_t, edge_hash_type> hEdgeIdx; // edge index 

	vertex_iterator_type vi, vie;
	uint32_t cnt = 0;
	for (boost::tie(vi, vie) = boost::vertices(this->m_graph); vi != vie; ++vi, ++cnt)
		hVertexIdx[*vi] = cnt;
	edge_iterator_type ei, eie;
	cnt = 0; 
	for (boost::tie(ei, eie) = boost::edges(this->m_graph); ei != eie; ++ei, ++cnt)
		hEdgeIdx[*ei] = cnt;

	// ILP environment
	GRBEnv env = GRBEnv();
	//mute the log from the LP solver
	env.set(GRB_IntParam_OutputFlag, 0);
	// set threads 
	if (this->m_threads > 0 && this->m_threads < std::numeric_limits<int32_t>::max())
		env.set(GRB_IntParam_Threads, this->m_threads);
	GRBModel opt_model = GRBModel(env);
	//set up the ILP variables
	vector<GRBVar> vVertexBit;
	vector<GRBVar> vEdgeBit;

	// vertex variables 
	vVertexBit.reserve(vertex_variable_num); 
	for (uint32_t i = 0; i != vertex_variable_num; ++i)
	{
		uint32_t vertex_idx = (i>>1);
		ostringstream oss; 
		oss << "v" << i;
		if (this->m_vColor[vertex_idx] >= 0 && this->m_vColor[vertex_idx] < this->m_color_num) // precolored 
		{
			int8_t color_bit;
			if ((i&1) == 0) color_bit = (this->m_vColor[vertex_idx]>>1)&1;
			else color_bit = this->m_vColor[vertex_idx]&1;
			vVertexBit.push_back(opt_model.addVar(color_bit, color_bit, color_bit, GRB_INTEGER, oss.str()));
		}
		else // uncolored 
			vVertexBit.push_back(opt_model.addVar(0, 1, 0, GRB_INTEGER, oss.str()));
	}

	// edge variables 
	vEdgeBit.reserve(edge_num);
	for (uint32_t i = 0; i != edge_num; ++i)
	{
		ostringstream oss;
		oss << "e" << i;
		vEdgeBit.push_back(opt_model.addVar(0, 1, 0, GRB_CONTINUOUS, oss.str()));
	}

	// update model 
	opt_model.update();

	// set up the objective 
	GRBLinExpr obj (0);
	for (boost::tie(ei, eie) = edges(this->m_graph); ei != eie; ++ei)
	{
		int32_t w = boost::get(boost::edge_weight, this->m_graph, *ei);
		if (w > 0) // weighted conflict 
			obj += w*vEdgeBit[hEdgeIdx[*ei]];
		else if (w < 0) // weighted stitch 
			obj += this->m_stitch_weight*(-w)*vEdgeBit[hEdgeIdx[*ei]];
	}
	opt_model.setObjective(obj, GRB_MINIMIZE);

	// set up the constraints
	uint32_t constr_num = 0;
	for (boost::tie(ei, eie) = boost::edges(this->m_graph); ei != eie; ++ei)
	{
		graph_vertex_type s = boost::source(*ei, this->m_graph);
		graph_vertex_type t = boost::target(*ei, this->m_graph);
		uint32_t sIdx = hVertexIdx[s];
		uint32_t tIdx = hVertexIdx[t];

		uint32_t vertex_idx1 = sIdx<<1;
		uint32_t vertex_idx2 = tIdx<<1;

		int32_t w = boost::get(boost::edge_weight, this->m_graph, *ei);
		uint32_t edge_idx = hEdgeIdx[*ei];

		char buf[100];
		string tmpConstr_name;
		if (w >= 0) // constraints for conflict edges 
		{
			sprintf(buf, "R%u", constr_num++);  
			opt_model.addConstr(
					vVertexBit[vertex_idx1] + vVertexBit[vertex_idx1+1] 
					+ vVertexBit[vertex_idx2] + vVertexBit[vertex_idx2+1] 
					+ vEdgeBit[edge_idx] >= 1
					, buf);

			sprintf(buf, "R%u", constr_num++);  
			opt_model.addConstr(
					1 - vVertexBit[vertex_idx1] + vVertexBit[vertex_idx1+1] 
					+ 1 - vVertexBit[vertex_idx2] + vVertexBit[vertex_idx2+1] 
					+ vEdgeBit[edge_idx] >= 1
					, buf);

			sprintf(buf, "R%u", constr_num++);  
			opt_model.addConstr(
					vVertexBit[vertex_idx1] + 1 - vVertexBit[vertex_idx1+1] 
					+ vVertexBit[vertex_idx2] + 1 - vVertexBit[vertex_idx2+1] 
					+ vEdgeBit[edge_idx] >= 1
					, buf);

			sprintf(buf, "R%u", constr_num++);  
			opt_model.addConstr(
					1 - vVertexBit[vertex_idx1] + 1 - vVertexBit[vertex_idx1+1] 
					+ 1 - vVertexBit[vertex_idx2] + 1 - vVertexBit[vertex_idx2+1] 
					+ vEdgeBit[edge_idx] >= 1
					, buf);

		}
		else // constraints for stitch edges 
		{
			sprintf(buf, "R%u", constr_num++);  
			opt_model.addConstr(
					vVertexBit[vertex_idx1] - vVertexBit[vertex_idx2] - vEdgeBit[edge_idx] <= 0
					, buf);

			sprintf(buf, "R%u", constr_num++);  
			opt_model.addConstr(
					vVertexBit[vertex_idx2] - vVertexBit[vertex_idx1] - vEdgeBit[edge_idx] <= 0
					, buf);

			sprintf(buf, "R%u", constr_num++);  
			opt_model.addConstr(
					vVertexBit[vertex_idx1+1] - vVertexBit[vertex_idx2+1] - vEdgeBit[edge_idx] <= 0
					, buf);      

			sprintf(buf, "R%u", constr_num++);  
			opt_model.addConstr(
					vVertexBit[vertex_idx2+1] - vVertexBit[vertex_idx1+1] - vEdgeBit[edge_idx] <= 0
					, buf);
		}
	}

	// additional constraints for 3-coloring 
	if (this->m_color_num == base_type::THREE)
	{
		char buf[100];
		for(uint32_t k = 0; k != vertex_variable_num; k += 2) 
		{
			sprintf(buf, "R%u", constr_num++);  
			opt_model.addConstr(vVertexBit[k] + vVertexBit[k+1] <= 1, buf);
		}
	}

	//optimize model 
	opt_model.update();
	opt_model.optimize();
#ifdef DEBUG_ILPCOLORING
	opt_model.write("graph.lp");
	opt_model.write("graph.sol");
#endif 
	int32_t opt_status = opt_model.get(GRB_IntAttr_Status);
	if(opt_status == GRB_INFEASIBLE) 
	{
		cout << "ERROR: The model is infeasible... EXIT" << endl;
		exit(1);
	}

	// collect coloring solution 
	for (uint32_t k = 0; k != vertex_variable_num; k += 2)
	{
		int8_t color = (vVertexBit[k].get(GRB_DoubleAttr_X)*2)+vVertexBit[k+1].get(GRB_DoubleAttr_X);
		uint32_t vertex_idx = (k>>1);

		assert(color >= 0 && color < this->m_color_num);
		if (this->m_vColor[vertex_idx] >= 0 && this->m_vColor[vertex_idx] < this->m_color_num) // check precolored vertex 
			assert(this->m_vColor[vertex_idx] == color);
		else // assign color to uncolored vertex 
			this->m_vColor[vertex_idx] = color;
	}

	// return objective value 
	return opt_model.get(GRB_DoubleAttr_ObjVal);
}

}}} // namespace limbo // namespace algorithms // namespace coloring

#endif
