/*
 *      DecisionTree.cpp
 *
 *      Copyright 2008 Christian Muise <christian.muise@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include "DecisionTree.h"
#include "MainSolver.h"

/////////////
// DT_Node //
/////////////
set<DTNode *>::iterator DTNode::getChildrenBegin()
{
	return children.begin();
}

set<DTNode *>::iterator DTNode::getChildrenEnd()
{
	return children.end();
}

set<int>::iterator DTNode::getVariablesBegin()
{
	return variables.begin();
}

set<int>::iterator DTNode::getVariablesEnd()
{
	return variables.end();
}

DTNode * DTNode::onlyChild()
{
	return *(children.begin());
}

int DTNode::numChildren()
{
	return children.size();
}

int DTNode::numVariables()
{
	return variables.size();
}

bool DTNode::hasVariable(int var)
{
	return variables.find(var) != variables.end();
}

bool DTNode::hasChild(DTNode * child)
{
	return children.find(child) != children.end();
}

int DTNode::numParents()
{
	return parents.size();
}

int DTNode::getID()
{
	return id;
}

DT_NodeType DTNode::getType()
{
	return type;
}

int DTNode::getVal()
{
	return val;
}

bool DTNode::isBottom()
{
	return type == DT_BOTTOM;
}

bool DTNode::isTop()
{
	return type == DT_TOP;
}

bool DTNode::isValid()
{
	return ((type == DT_TOP) || (type == DT_BOTTOM) || (type == DT_LIT)
			|| (type == DT_AND) || (type == DT_OR));
}

void DTNode::topIfy()
{
	if (DT_LIT == type)
		toSTDOUT("Warning: Converting DT_LIT to DT_TOP!!" << endl);

	type = DT_TOP;
}

void DTNode::botIfy()
{
	if (DT_LIT == type)
		toSTDOUT("Warning: Converting DT_LIT to DT_BOTTOM!!" << endl);

	type = DT_BOTTOM;
}

// Add parent
void DTNode::addParent(DTNode * newParent, bool link)
{
	if (!newParent->isValid())
		toSTDOUT("Error: Attempting to add invalid parent." << endl);

	parents.insert(newParent);
	if (link)
		newParent->addChild(this);
}

// Add a child
void DTNode::addChild(DTNode * newChild, bool link)
{
	if (!newChild->isValid())
		toSTDOUT("Error: Attempting to add invalid child." << endl);

	if (!firstNode)
		firstNode = newChild;
	else if (!secondNode)
		secondNode = newChild;

	children.insert(newChild);
	if (link)
		newChild->addParent(this);
}

// A child was deleted
bool DTNode::childDeleted(DTNode * oldChild)
{
	if (!oldChild->isValid())
		toSTDOUT("Error: Attempting to delete invalid child." << endl);

	if (children.find(oldChild) == children.end())
	{
		return false;
	}
	
	if (firstNode == oldChild)
		firstNode = NULL;
	
	if (secondNode == oldChild)
		secondNode = NULL;

	children.erase(oldChild);

	return true;
}

// A parent was deleted
bool DTNode::parentDeleted(DTNode * oldParent)
{
	if (!oldParent->isValid())
		toSTDOUT("Error: Attempting to delete invalid parent." << endl);

	if (parents.find(oldParent) == parents.end())
	{
		return false;
	}

	parents.erase(oldParent);

	return true;
}

void DTNode::compressNode()
{

	if (checked)
		return;
	checked = true;

	set<DTNode *>::iterator it;
	set<DTNode *>::iterator it2;
	bool found = false;
	bool allTrue, allFalse;

	switch (type)
	{
	case DT_AND:
		// First we recurse
		for (it = children.begin(); it != children.end(); it++)
		{
			(*it)->compressNode();
		}

		// Next we collapse if all the children are True
		allTrue = true;
		for (it = children.begin(); it != children.end(); it++)
		{
			if (!((*it)->isTop()))
				allTrue = false;
		}
		if (allTrue)
		{
			// Remove the children
			for (it = children.begin(); it != children.end(); it++)
			{
				(*it)->parentDeleted(this);

				if ((0 == (*it)->numParents()) && ((*it)->getType() != DT_LIT))
					delete *it;

			}
			children.clear();
			type = DT_TOP;
			return;
		}

		// Next collapse if the child has only one child itself
		found = true;
		while (found)
		{
			found = false;
			for (it = children.begin(); it != children.end() && !found; it++)
			{
				if (1 == (*it)->numChildren())
				{
					// Mark as found
					found = true;

					DTNode * oldNode = (*it);

					childDeleted(oldNode);
					oldNode->parentDeleted(this);

					addChild(oldNode->onlyChild(), true);

					// Get rid of the oldNode if this was the only parent
					if ((0 == oldNode->numParents()) && (oldNode->getType()
							!= DT_LIT))
					{
						delete oldNode;
					}
				}
			}
		}

		// Now we check if a False child exists (which falsifies this node)
		for (it = children.begin(); it != children.end(); it++)
		{
			if (DT_BOTTOM == (*it)->getType())
			{
				// Remove the children
				for (it2 = children.begin(); it2 != children.end(); it2++)
				{
					(*it2)->parentDeleted(this);

					if ((0 == (*it2)->numParents()) && ((*it2)->getType()
							!= DT_LIT))
						delete *it2;

				}
				children.clear();
				type = DT_BOTTOM;
				return;
			}
		}

		// Get rid of empty OR and AND children
		found = true;
		while (found)
		{
			found = false;
			for (it = children.begin(); it != children.end() && !found; it++)
			{
				if (0 == (*it)->numChildren())
				{
					if ((DT_AND == (*it)->getType()) || (DT_OR
							== (*it)->getType()))
					{
						found = true;

						(*it)->parentDeleted(this);
						childDeleted(*it);

						if ((0 == (*it)->numParents()) && ((*it)->getType()
								!= DT_LIT))
							delete *it;
					}
				}
			}
		}

		// Get rid of the True children
		found = true;
		while (found)
		{
			found = false;
			for (it = children.begin(); it != children.end() && !found; it++)
			{
				if (DT_TOP == (*it)->getType())
				{
					found = true;

					(*it)->parentDeleted(this);
					childDeleted(*it);

					if ((0 == (*it)->numParents()) && ((*it)->getType()
							!= DT_LIT))
						delete *it;
				}
			}
		}

		// Finally check for 'AND' children (we want the final graph to be an AND-OR tree)
		found = true;
		while (found)
		{
			found = false;
			for (it = children.begin(); it != children.end() && !found; it++)
			{
				if (DT_AND == (*it)->getType())
				{

					DTNode * oldNode = (*it);
					oldNode->parentDeleted(this);
					childDeleted(oldNode);

					// Add links from the current node to the grandchildren
					for (it2 = oldNode->getChildrenBegin(); it2
							!= oldNode->getChildrenEnd(); it2++)
					{
						if (!(*it2)->isValid())
						{
							toSTDOUT("Error: Found bad and grandchild!!" << endl);
						}

						addChild(*it2, true);
					}

					// Remove oldNode if it no longer has a parent
					if ((0 == oldNode->numParents()) && (oldNode->getType()
							!= DT_LIT))
						delete oldNode;

					// Mark as found
					found = true;
				}
			}
		}
		break;
	case DT_OR:

		// First we recurse
		for (it = children.begin(); it != children.end(); it++)
		{
			(*it)->compressNode();
		}

		// Next we collapse if all the children are False
		allFalse = true;
		for (it = children.begin(); it != children.end(); it++)
		{
			if (!((*it)->isBottom()))
				allFalse = false;
		}
		if (allFalse)
		{
			// Remove the children
			for (it = children.begin(); it != children.end(); it++)
			{
				(*it)->parentDeleted(this);
				if ((0 == (*it)->numParents()) && ((*it)->getType() != DT_LIT))
					delete *it;
			}
			children.clear();
			type = DT_BOTTOM;
			return;
		}

		// Next collapse if the child has only one child
		found = true;
		while (found)
		{
			found = false;
			for (it = children.begin(); it != children.end() && !found; it++)
			{
				if (1 == (*it)->numChildren())
				{

					// Mark as found
					found = true;

					DTNode * oldNode = (*it);

					childDeleted(oldNode);
					oldNode->parentDeleted(this);

					addChild(oldNode->onlyChild(), true);

					// Get rid of the oldNode if this was the only parent
					if ((0 == oldNode->numParents()) && (oldNode->getType()
							!= DT_LIT))
					{
						delete oldNode;
					}
				}
			}
		}

		// Now we check if a True child exists (which trivializes this node)
		for (it = children.begin(); it != children.end(); it++)
		{
			if (DT_TOP == (*it)->getType())
			{
				// Remove the children
				for (it2 = children.begin(); it2 != children.end(); it2++)
				{
					(*it2)->parentDeleted(this);
					if ((0 == (*it2)->numParents()) && ((*it2)->getType()
							!= DT_LIT))
						delete *it2;
				}
				children.clear();
				type = DT_TOP;
				return;
			}
		}

		// Get rid of empty OR and AND children
		found = true;
		while (found)
		{
			found = false;
			for (it = children.begin(); it != children.end() && !found; it++)
			{
				if (0 == (*it)->numChildren())
				{
					if ((DT_AND == (*it)->getType()) || (DT_OR
							== (*it)->getType()))
					{
						found = true;

						(*it)->parentDeleted(this);
						childDeleted(*it);

						if ((0 == (*it)->numParents()) && ((*it)->getType()
								!= DT_LIT))
							delete *it;
					}
				}
			}
		}

		// Get rid of the False children
		found = true;
		while (found)
		{
			found = false;
			for (it = children.begin(); it != children.end() && !found; it++)
			{
				if (DT_BOTTOM == (*it)->getType())
				{
					found = true;

					(*it)->parentDeleted(this);
					childDeleted(*it);

					if ((0 == (*it)->numParents()) && ((*it)->getType()
							!= DT_LIT))
						delete *it;
				}
			}
		}

		// Check for 'OR' children (we want the final graph to be an AND-OR tree)
		found = true;
		while (found)
		{
			found = false;
			for (it = children.begin(); it != children.end() && !found; it++)
			{
				if (DT_OR == (*it)->getType())
				{
					DTNode * oldNode = (*it);
					oldNode->parentDeleted(this);
					childDeleted(oldNode);

					// Add links from the current node to the grandchildren
					for (it2 = oldNode->getChildrenBegin(); it2
							!= oldNode->getChildrenEnd(); it2++)
					{
						if (!(*it2)->isValid())
						{
							toSTDOUT("Error: Found bad or grandchild!!" << endl);
						}

						addChild(*it2, true);
					}

					// Remove oldNode if it no longer has a parent
					if ((0 == oldNode->numParents()) && (oldNode->getType()
							!= DT_LIT))
						delete oldNode;

					// Mark as found
					found = true;
				}
			}
		}
		
		// Finally, revert to true if two children are opposite literals
		{
			set<int> litChildren;
			found = false;
			for (it = children.begin(); it != children.end(); it++) {
				if (DT_LIT == (*it)->getType()) {
					if (litChildren.find(-1 * (*it)->getVal()) != litChildren.end())
						found = true;
					litChildren.insert((*it)->getVal());
				}
			}
			if (found) {
				topIfy();
			}
		}
		break;
		/* Leaf node */
	case DT_BOTTOM:
	case DT_TOP:
	case DT_LIT:
	default:
		return;
	}
}

int DTNode::count(bool isRoot)
{
	if (checked)
		return 1;
	checked = true;

	set<DTNode *>::iterator it;
	int sum = isRoot ? 0 : 1;
	set<int> childrenSeen;
	set<int> litsSeen;

	// First we recurse
	for (it = children.begin(); it != children.end(); it++)
	{
		if (DT_LIT == (*it)->getType())
		{
			int lit_num = (*it)->getVal();
			if (litsSeen.find(lit_num) == litsSeen.end())
			{
				litsSeen.insert(lit_num);
				sum += 1;
			}

		}
		else
		{
			int node_id = (*it)->getID();
			if (childrenSeen.find(node_id) == childrenSeen.end())
			{
				childrenSeen.insert(node_id);
				sum += (*it)->count(false);
			}
		}
	}

	return sum;
}


void DTNode::uncheck(int unID)
{
	if (uncheckID == unID)
		return;

	set<DTNode *>::iterator it;
	checked = false;
	uncheckID = unID;

	// Recurse
	for (it = children.begin(); it != children.end(); it++)
	{
		(*it)->uncheck(unID);
	}
}

bool DTNode::validate()
{
	set<DTNode *>::iterator it;

	for (it = parents.begin(); it != parents.end(); it++)
	{
		if (!(*it)->isValid())
		{
			toSTDOUT("Error: Node has invalid parent." << endl);
			return false;
		}
		if (!(*it)->hasChild(this))
		{
			toSTDOUT("Error: Node's parent doesn't have child." << endl);
			return false;
		}
	}

	// Recurse
	for (it = children.begin(); it != children.end(); it++)
	{
		if (!(*it)->isValid())
		{
			toSTDOUT("Error: Node has invalid child." << endl);
			return false;
		}
		else
		{
			if (!(*it)->validate())
				return false;
		}
	}
	return true;
}

// Reset the node
// Note: This should only happen with an AND node,
//        and the first child is the literal.
void DTNode::reset()
{
	set<DTNode *>::iterator it;

	for (it = children.begin(); it != children.end(); it++)
		(*it)->parentDeleted(this);

	children.erase(children.begin(), children.end());

	// Save the first node
	addChild(firstNode, true);
}

// Printout
void DTNode::print(int depth)
{
	toSTDOUT("(" << id << "-" << type << ":");
	if (DT_LIT == type)
	{
		toSTDOUT(val);
	}

	if (depth != 0)
	{
		set<DTNode *>::iterator it;
		for (it = children.begin(); it != children.end(); it++)
		{
			(*it)->print(depth - 1);
		}
	}
	toSTDOUT(")");
}

// Prep the nnf format
void DTNode::prepNNF(vector<DTNode*> * nodeList)
{
	if (nnfID != -1)
		return;

	if (((DT_TOP == type) || (DT_BOTTOM == type)) && !(CSolverConf::smoothNNF))
	{
		toSTDOUT("Error: type of DTNode is either top or bottom." << endl);
		return;
	}

	if (DT_LIT != type)
	{
		set<DTNode *>::iterator it;
		for (it = children.begin(); it != children.end(); it++)
		{
			(*it)->prepNNF(nodeList);
		}
	}

	// Add / id this DT node
	nnfID = nodeList->size();
	nodeList->push_back(this);
}


// Output the nnf format
void DTNode::genNNF(ostream & out)
{
	if (DT_LIT == type) 
		out << "L " << val << endl;
	else if (DT_TOP == type)
		out << "A 0" << endl;
	else if (DT_BOTTOM == type)
		out << "O 0 0" << endl;
	else if (DT_AND == type)
	{
		out << "A " << children.size();

		set<DTNode *>::iterator it;
		for (it = children.begin(); it != children.end(); it++)
			out << " " << (*it)->nnfID;

		out << endl;
	}
	else if (DT_OR == type)
	{
		//Dimitar Sht. Shterionov: ignoring negative values on OR nodes
		if (choiceVar > 0)
			out << "O " << choiceVar << " " << children.size();
		else
			out << "O 0 " << children.size();

		if (2 != children.size())
			toSTDOUT("Error: Or node with " << children.size() << " children.");

		set<DTNode *>::iterator it;
		for (it = children.begin(); it != children.end(); it++)
			out << " " << (*it)->nnfID;

		out << endl;
	}
}

bool DTNode::checkCycle(int sourceID, bool first)
{
	if (!first and (getID() == sourceID))
	{
		toSTDOUT("FOUND CYCLE:" << endl);
		toSTDOUT(sourceID);
		return true;
	}

	set<DTNode *>::iterator it;
	for (it = children.begin(); it != children.end(); it++)
	{
		if ((*it)->checkCycle(sourceID, false)) {
			toSTDOUT(" " << getID());
			return true;
		}
	}
	return false;
}


/*************************************
 * Smoothing functionality           *
 * Original: Dimitar Sht. Shterionov *
 * Modified: Christian Muise         *
 *************************************/

void DTNode::smooth(int &num_nodes, CMainSolver &solver, set<int> &literals)
{
	// Checked means that it is already smoothed
	if (checked)
		return;
	checked = true;
	variables.clear();

	// If this is a literal, we just add the variable
	if (DT_LIT == type) {
		int var = (val < 0) ? -1 * val : val;
		variables.insert(var);
		literals.insert(val);
		return;
	}

	// First we recurse and record the variables below
	set<DTNode *>::iterator it;
	for (it = children.begin(); it != children.end(); it++)
	{
		(*it)->smooth(num_nodes, solver, literals);
		variables.insert((*it)->getVariablesBegin(), (*it)->getVariablesEnd());
	}

	// If this is an AND node, there's nothing to do
	if (DT_AND == type)
		return;

	set<DTNode *> toAdd;
	set<DTNode *> toRemove;
	
	// We must make sure that every child has all of the variables (smoothness)
	for (it = children.begin(); it != children.end(); it++)
	{
		// If the counts are the same, then it is already smooth
		if (variables.size() != (*it)->numVariables()) {
			// Create the new AND child
			DTNode* newAnd = new DTNode(DT_AND, num_nodes++);

			(*it)->parentDeleted(this);
			
			toAdd.insert(newAnd);
			toRemove.insert(*it);
			
			newAnd->addChild(*it, true);
			
			// Add all of the missing variables
			set<int>::iterator var_it;
			for (var_it = variables.begin(); var_it != variables.end(); var_it++)
			{
				int var = *var_it;
				if (!((*it)->hasVariable(var)))
				{
					DTNode* newOr = new DTNode(DT_OR, num_nodes++);
					newAnd->addChild(newOr, true);
					newOr->addChild(solver.get_lit_node_full(var), true);
					newOr->addChild(solver.get_lit_node_full(-1 * var), true);
				}
			}
			// Record the new values
			newAnd->smooth(num_nodes, solver, literals);
		}
	}
	
	for (it = toRemove.begin(); it != toRemove.end(); it++)
		childDeleted(*it);
	
	for (it = toAdd.begin(); it != toAdd.end(); it++)
		addChild(*it, true);
	
	return;
}

void DTNode::sub_parents(DTNode *newChild) {
	for (set<DTNode *>::iterator it = parents.begin(); it != parents.end(); it++) {
		(*it)->childDeleted(this);
		(*it)->addChild(newChild, true);
	}
	parents.clear();
}

