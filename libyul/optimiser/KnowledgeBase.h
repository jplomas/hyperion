/*
	This file is part of hyperion.

	hyperion is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	hyperion is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with hyperion.  If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-3.0
/**
 * Class that can answer questions about values of variables and their relations.
 */

#pragma once

#include <libyul/ASTForward.h>
#include <libyul/YulString.h>

#include <libhyputil/Common.h>
#include <libhyputil/Numeric.h>

#include <map>
#include <functional>
#include <tuple>

namespace hyperion::yul
{

struct Dialect;
struct AssignedValue;

/**
 * Class that can answer questions about values of variables and their relations.
 *
 * Requires a callback that returns the current value of the variable.
 * The value can change during the lifetime of the KnowledgeBase, so the code it is
 * used on does not need to be in SSA form. In that case, the caller has to ensure:
 *  - The values are movable expressions.
 *  - ``variableChanged`` is called whenever the value of a variable changes.
 *  - A value only references variables that have not changed since it was assigned.
 *    In particular, it does not reference the variable it is assigned to.
 *    Values are evaluated lazily. So if a referenced variable changes, the value
 *    has to be forgotten, otherwise it would be evaluated using the new value of
 *    the referenced variable. This also rules out cycles, which is needed
 *    for exploration to terminate.
 * The DataFlowAnalyzer ensures this in ``handleAssignment`` and ``clearValues``.
 *
 * There is a constructor to provide all SSA values right at the beginning.
 * If you use this, the KnowledgeBase will be slightly more efficient.
 *
 * Internally, tries to find groups of variables that have a mutual constant
 * difference and stores these differences always relative to a specific
 * representative variable of the group.
 * Variables are versioned, so these differences never have to be invalidated,
 * in particular, after a change, the variable is treated as a new one.
 *
 * There is a special group which is the constant values. Those use the
 * empty YulString as representative "variable".
 */
class KnowledgeBase
{
public:
	/// Constructor for arbitrary value callback that allows for variable values
	/// to change in between calls to functions of this class.
	KnowledgeBase(std::function<AssignedValue const*(YulString)> _variableValues):
		m_variableValues(std::move(_variableValues))
	{}
	/// Constructor to use if source code is in SSA form and values are constant.
	KnowledgeBase(std::map<YulString, AssignedValue> const& _ssaValues);

	bool knownToBeDifferent(YulString _a, YulString _b);
	std::optional<u512> differenceIfKnownConstant(YulString _a, YulString _b);
	bool knownToBeDifferentByAtLeastWordSize(YulString _a, YulString _b);
	bool knownToBeZero(YulString _a);
	std::optional<u512> valueIfKnownConstant(YulString _a);
	std::optional<u512> valueIfKnownConstant(Expression const& _expression);

	/// Has to be called whenever the value of @a _var changes.
	void variableChanged(YulString _var);

private:
	struct VersionedVariable
	{
		YulString name;
		size_t version = 0;

		bool operator==(VersionedVariable const& _other) const
		{
			return name == _other.name && version == _other.version;
		}
		bool operator<(VersionedVariable const& _other) const
		{
			return std::tie(name, version) < std::tie(_other.name, _other.version);
		}
	};

	/**
	 * Constant offset relative to a reference variable, or absolute constant if the
	 * reference variable is the empty YulString.
	 */
	struct VariableOffset
	{
		VersionedVariable reference;
		u512 offset;

		bool isAbsolute() const
		{
			return reference.name.empty();
		}

		std::optional<u512> absoluteValue() const
		{
			if (isAbsolute())
				return offset;
			else
				return std::nullopt;
		}
	};

	struct CachedOffset
	{
		VariableOffset offset;
		/// Value of the variable the offset was computed from, nullptr if it was unknown
		Expression const* computedFrom = nullptr;
	};

	/// @returns @a _var at its current version, which is zero if it never changed.
	VersionedVariable withCurrentVersion(YulString _var) const;

	VariableOffset explore(YulString _var);
	std::optional<VariableOffset> explore(Expression const& _value);

	/// If true, we can assume that variable values never change and skip some steps.
	bool m_valuesAreSSA = false;
	/// Callback to retrieve the current value of a variable.
	std::function<AssignedValue const*(YulString)> m_variableValues;

	/// Offsets for each version of a variable to one representative per group.
	/// The empty string is the representative of the constant value zero.
	std::map<VersionedVariable, CachedOffset> m_offsets;

	/// Current version of each variable that changed at least once.
	std::map<YulString, size_t> m_currentVersions;
};

}
